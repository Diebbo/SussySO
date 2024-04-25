/*
 * uRISCV - A general purpose computer system simulator
 *
 * Copyright (C) 2004 Mauro Morsiani
 * Copyright (C) 2020 Mattia Biondi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

/****************************************************************************
 *
 * This module implements the SystemBus object class.
 *
 * A SystemBus connects a Processor object to memory and devices.
 * It satisfies the Processor requests to read and write memory positions,
 * and simulates interrupts generation and other external exceptions.
 * SystemBus defines the physical memory map of the system, mapping RamSpace
 * and BiosSpace objects and Device registers to "physical" addresses in
 * memory.
 * It also implements the system clock and timer, and keeps track of some
 * important system constants.  All this information is mapped in the "bus
 * device register area", at the start of the device register area.
 *
 * On creation, it initializes the device and memory subsystems
 * as set in the simulator configuration (contained in the SetupInfo
 * object); on destruction, it removes them.
 *
 * SystemBus notifies all changes to memory location to Watch controlling
 * object, to track memory accesses for simulation brkpt/susp/trace handling
 *
 ****************************************************************************/

#include "uriscv/systembus.h"

#include <assert.h>

#include "uriscv/arch.h"
#include "uriscv/blockdev.h"
#include "uriscv/blockdev_params.h"
#include "uriscv/const.h"
#include "uriscv/cpu.h"
#include "uriscv/device.h"
#include "uriscv/error.h"
#include "uriscv/event.h"
#include "uriscv/machine.h"
#include "uriscv/machine_config.h"
#include "uriscv/memspace.h"
#include "uriscv/mp_controller.h"
#include "uriscv/mpic.h"
#include "uriscv/processor.h"
#include "uriscv/time_stamp.h"
#include "uriscv/types.h"
#include "uriscv/utility.h"

// This macro converts a byte address into a word address (minus offset)
#define CONVERT(ad, bs) ((ad - bs) >> WORDSHIFT)

class DeviceAreaAddress {
public:
  DeviceAreaAddress(Word paddr) : pa(paddr) {
    assert(MMIO_BASE <= paddr && paddr < MMIO_END);
  }

  DeviceAreaAddress(unsigned int line, unsigned int device, unsigned int field)
      : pa(DEV_REG_ADDR(line + DEV_IL_START, device) + field * WS) {
    assert(line < N_EXT_IL && device < N_DEV_PER_IL && field < DEV_REG_SIZE_W);
  }

  Word address() const { return pa; }

  unsigned int regIndex() const { return wordIndex() / DEV_REG_SIZE_W; }
  unsigned int line() const { return regIndex() / N_DEV_PER_IL; }
  unsigned int device() const { return regIndex() % N_DEV_PER_IL; }
  unsigned int field() const { return wordIndex() % DEV_REG_SIZE_W; }

private:
  unsigned int wordIndex() const { return (pa - DEV_REG_START) >> 2; }

  Word pa;
};

SystemBus::SystemBus(const MachineConfig *conf, Machine *machine)
    : config(conf), machine(machine), pic(new InterruptController(conf, this)),
      mpController(new MPController(conf, machine)) {
  tod = UINT64_C(0);
  timer = MAXWORDVAL;
  eventQ = new EventQueue();

  const char *coreFile = NULL;
  if (config->isLoadCoreEnabled())
    coreFile = config->getROM(ROM_TYPE_CORE).c_str();
  ram = new RamSpace(config->getRamSize() * FRAMESIZE, coreFile);

  biosdata = new RamSpace(BIOSDATASIZE, NULL);
  bios = new BiosSpace(config->getROM(ROM_TYPE_BIOS).c_str());
  boot = new BiosSpace(config->getROM(ROM_TYPE_BOOT).c_str());

  // Create devices and initialize registers used for interrupt
  // handling.
  intPendMask = 0UL;
  for (unsigned intl = 0; intl < N_EXT_IL; intl++) {
    instDevTable[intl] = 0UL;
    for (unsigned int devNo = 0; devNo < N_DEV_PER_IL; devNo++) {
      devTable[intl][devNo] = makeDev(intl, devNo);
      if (devTable[intl][devNo]->Type() != NULLDEV)
        instDevTable[intl] = SetBit(instDevTable[intl], devNo);
    }
  }
}

// This method deletes a SystemBus object and all related structures
SystemBus::~SystemBus() {
  delete eventQ;

  delete ram;
  delete biosdata;
  delete bios;
  delete boot;

  for (unsigned int intl = 0; intl < DEVINTUSED; intl++)
    for (unsigned int dnum = 0; dnum < DEVPERINT; dnum++)
      delete devTable[intl][dnum];
}

// This method increments system clock and decrements interval timer;
// on timer underflow (0 -> FFFFFFFF transition) a interrupt is
// generated.  Event queue is checked against the current clock value
// and device operations are completed if needed; all memory changes
// are notified to Watch control object
void SystemBus::ClockTick() {
  tod++;

  // both registers signal "change" because they are conceptually one
  machine->HandleBusAccess(BUS_REG_TOD_HI, WRITE, NULL);
  machine->HandleBusAccess(BUS_REG_TOD_LO, WRITE, NULL);

  // Update interval timer
  if (UnsSub(&timer, timer, 1)) {
    pic->StartIRQ(IL_TIMER);
  }
  machine->HandleBusAccess(BUS_REG_TIMER, WRITE, NULL);

  // Scan the event queue
  while (!eventQ->IsEmpty() && eventQ->nextDeadline() <= tod) {
    (eventQ->nextCallback())();
    eventQ->RemoveHead();
  }
}

uint32_t SystemBus::IdleCycles() const {
  if (eventQ->IsEmpty())
    return timer;

  const uint64_t et = eventQ->nextDeadline();
  if (et > tod)
    return std::min(timer, (uint32_t)(et - tod - 1));
  else
    return 0;
}

void SystemBus::Skip(uint32_t cycles) {
  tod += cycles;
  machine->HandleBusAccess(BUS_REG_TOD_HI, WRITE, NULL);
  machine->HandleBusAccess(BUS_REG_TOD_LO, WRITE, NULL);

  timer -= cycles;
  machine->HandleBusAccess(BUS_REG_TIMER, WRITE, NULL);
}

void SystemBus::setToDHI(Word hi) { TimeStamp::setHi(tod, hi); }

void SystemBus::setToDLO(Word lo) { TimeStamp::setLo(tod, lo); }

void SystemBus::setTimer(Word time) { timer = time; }

// This method reads a data word from memory at address addr, returning it
// thru datap pointer. It also returns TRUE if the address was invalid and
// an exception was caused, FALSE otherwise, and signals memory access to
// Watch control object
bool SystemBus::DataRead(Word addr, Word *datap, Processor *cpu) {
  machine->HandleBusAccess(addr, READ, cpu);

  if (busRead(addr, datap, cpu)) {
    // address invalid: signal exception to processor
    cpu->SignalExc(EXC_LAF);
    return true;
  }

  return false;
}

//
// These methods allow Watch to inspect or modify single memory locations;
// they return TRUE if address is invalid or memory cannot be altered, and
// FALSE otherwise
//

bool SystemBus::WatchRead(Word addr, Word *datap) {
  return busRead(addr, datap, machine->getProcessor(0));
}

bool SystemBus::WatchWrite(Word addr, Word data) {
  return busWrite(addr, data, machine->getProcessor(0));
}

// This method writes the data word at physical addr in RAM memory or device
// register area.  Writes to BIOS or BOOT areas cause a DBEXCEPTION (no
// writes allowed). It returns TRUE if an exception was caused, FALSE
// otherwise, and notifies access to Watch control object
bool SystemBus::DataWrite(Word addr, Word data, Processor *proc) {
  machine->HandleBusAccess(addr, WRITE, proc);

  if (busWrite(addr, data, proc)) {
    // data write is out of valid write bounds
    proc->SignalExc(EXC_SAF);
    return true;
  }
  return false;
}

// This method transfers a block from or to memory, starting with address
// startAddr; it returns TRUE is transfer was not successful (non-existent
// memory, read-only memory, unaligned addresses), FALSE otherwise.
// It notifies too the memory accesses to Watch control object
bool SystemBus::DMATransfer(Block *blk, Word startAddr, bool toMemory) {
  if (BADADDR(startAddr))
    return true;

  bool error = false;

  if (toMemory) {
    for (Word ofs = 0; ofs < BLOCKSIZE && !error; ofs++) {
      error = busWrite(startAddr + (ofs * WORDLEN), blk->getWord(ofs));
      machine->HandleBusAccess(startAddr + (ofs * WORDLEN), WRITE, NULL);
    }
  } else {
    Word val;
    for (Word ofs = 0; ofs < BLOCKSIZE && !error; ofs++) {
      error = busRead(startAddr + (ofs * WORDLEN), &val);
      machine->HandleBusAccess(startAddr + (ofs * WORDLEN), READ, NULL);
      blk->setWord(ofs, val);
    }
  }

  return error;
}

// This method transfers a partial block from or to memory, starting with
// address startAddr; it returns TRUE is transfer was not successful
// (non-existent memory, read-only memory, unaligned addresses), FALSE
// otherwise. It notifies too the memory accesses to Watch control object
bool SystemBus::DMAVarTransfer(Block *blk, Word startAddr, Word byteLength,
                               bool toMemory) {
  // fit bytes into words
  Word length;
  if (byteLength % WORDLEN)
    length = (byteLength / WORDLEN) + 1;
  else
    length = byteLength / WORDLEN;

  if (BADADDR(startAddr) || length > BLOCKSIZE)
    return true;

  bool error = false;

  if (toMemory) {
    for (Word ofs = 0; ofs < length && !error; ofs++) {
      error = busWrite(startAddr + (ofs * WORDLEN), blk->getWord(ofs));
      machine->HandleBusAccess(startAddr + (ofs * WORDLEN), WRITE, NULL);
    }
  } else {
    Word val;
    for (Word ofs = 0; ofs < length && !error; ofs++) {
      error = busRead(startAddr + (ofs * WORDLEN), &val);
      machine->HandleBusAccess(startAddr + (ofs * WORDLEN), READ, NULL);
      blk->setWord(ofs, val);
    }
  }

  return error;
}

// This method reads a istruction from memory at address addr, returning
// it thru istrp pointer. It also returns TRUE if the address was invalid and
// an exception was caused, FALSE otherwise, and notifies Watch
bool SystemBus::InstrRead(Word addr, Word *instrp, Processor *proc) {
  machine->HandleBusAccess(addr, EXEC, proc);

  if (busRead(addr, instrp)) {
    // address invalid: signal exception to processor
    proc->SignalExc(EXC_IAF);
    return true;
  } else {
    // address was valid
    return false;
  }
}

bool SystemBus::InstrReadGDB(Word addr, Word *instrp, Processor *proc) {
  if (busRead(addr, instrp)) {
    // address invalid: signal exception to processor
    return true;
  } else {
    // address was valid
    return false;
  }
}

// This method inserts in the eventQ a event that must happen
// at (current system time) + delay
uint64_t SystemBus::scheduleEvent(uint64_t delay, Event::Callback callback) {
  return eventQ->InsertQ(tod, delay, callback);
}

void SystemBus::IntReq(unsigned int intl, unsigned int devNum) {
  pic->StartIRQ(DEV_IL_START + intl, devNum);
}

void SystemBus::IntAck(unsigned int intl, unsigned int devNum) {
  pic->EndIRQ(DEV_IL_START + intl, devNum);
}

// This method returns the current interrupt line status
Word SystemBus::getPendingInt(const Processor *cpu) {
  return pic->GetIP(cpu->Id());
}

void SystemBus::AssertIRQ(unsigned int il, unsigned int target) {
  machine->getProcessor(target)->AssertIRQ(il);
}

void SystemBus::DeassertIRQ(unsigned int il, unsigned int target) {
  machine->getProcessor(target)->DeassertIRQ(il);
}

// This method returns the Device object with given "coordinates"
Device *SystemBus::getDev(unsigned int intL, unsigned int dNum) {
  if (intL < DEVINTUSED && dNum < DEVPERINT)
    return (devTable[intL][dNum]);
  else {
    Panic("Unknown device specified in SystemBus::getDev()");
    // never returns
    return NULL;
  }
}

/****************************************************************************/
/* Definitions strictly local to the module.                                */
/****************************************************************************/

// This method reads the data at the address addr, and passes it back thru
// the datap pointer. It also return FALSE if the addr is valid, and TRUE
// otherwise
bool SystemBus::busRead(Word addr, Word *datap, Processor *cpu) {
  if (INBOUNDS(addr, RAMBASE, RAMBASE + ram->Size())) {
    *datap = ram->MemRead(CONVERT(addr, RAMBASE));
  } else if (INBOUNDS(addr, BIOSDATABASE, BIOSDATABASE + biosdata->Size())) {
    *datap = biosdata->MemRead(CONVERT(addr, BIOSDATABASE));
  } else if (INBOUNDS(addr, BIOSBASE, BIOSBASE + bios->Size())) {
    *datap = bios->MemRead(CONVERT(addr, BIOSBASE));
  } else if (INBOUNDS(addr, BOOTBASE, BOOTBASE + boot->Size())) {
    *datap = boot->MemRead(CONVERT(addr, BOOTBASE));
  } else if (INBOUNDS(addr, MMIO_BASE, MMIO_END)) {
    *datap = busRegRead(addr, cpu);
  } else {
    // address invalid: data read is out of bounds
    *datap = MAXWORDVAL;
    return true;
  }

  // address was valid
  return false;
}

// This method returns the value for the device field addressed in the "bus
// register area"
Word SystemBus::busRegRead(Word addr, Processor *cpu) {
  Word data;

  if (INBOUNDS(addr, DEV_REG_START, DEV_REG_END)) {
    // We're in the device register space
    DeviceAreaAddress da(addr);
    Device *device = devTable[da.line()][da.device()];
    data = device->ReadDevReg(da.field());
  } else if (INBOUNDS(addr, IDEV_BITMAP_BASE, IDEV_BITMAP_END)) {
    // We're in the "installed-devices bitmap" structure space
    unsigned int wordIndex = CONVERT(addr, IDEV_BITMAP_BASE);
    data = instDevTable[wordIndex];
  } else if (INBOUNDS(addr, CDEV_BITMAP_BASE, CDEV_BITMAP_END) ||
             INBOUNDS(addr, IRT_BASE, IRT_END) ||
             INBOUNDS(addr, CPUCTL_BASE, CPUCTL_END)) {
    data = pic->Read(addr, cpu);
  } else if (MCTL_BASE <= addr && addr < MCTL_END) {
    data = mpController->Read(addr, cpu);
  } else {
    // We're in the low "bus register area" space
    switch (addr) {
    case BUS_REG_TIME_SCALE:
      // number of ticks for a microsecond
      data = config->getClockRate();
      break;
    case BUS_REG_TOD_HI:
      data = getToDHI();
      break;
    case BUS_REG_TOD_LO:
      data = getToDLO();
      break;
    case BUS_REG_TIMER:
      data = timer;
      break;
    case BUS_REG_RAM_BASE:
      data = RAMBASE;
      break;
    case BUS_REG_RAM_SIZE:
      data = ram->Size();
      break;
    case BUS_REG_BIOS_BASE:
      data = BIOSBASE;
      break;
    case BUS_REG_BIOS_SIZE:
      data = bios->Size();
      break;
    case BUS_REG_BOOT_BASE:
      data = BOOTBASE;
      break;
    case BUS_REG_BOOT_SIZE:
      data = boot->Size();
      break;
    case TLB_FLOOR_ADDR:
      data = config->getTLBFloorAddress();
      break;
    default:
      // unmapped bus device register area:
      // read give 0, write has no effects
      data = 0UL;
      break;
    }
  }
  return data;
}

// This method accesses the system configuration and constructs
// the devices needed, linking them to SystemBus object
Device *SystemBus::makeDev(unsigned int intl, unsigned int dnum) {
  unsigned int devt;
  Device *dev;

  devt = config->getDeviceType(intl, dnum);

  switch (devt) {
  case PRNTDEV:
    dev = new PrinterDevice(this, config, intl, dnum);
    break;

  case TERMDEV:
    dev = new TerminalDevice(this, config, intl, dnum);
    break;

  case ETHDEV:
    dev = new EthDevice(this, config, intl, dnum);
    break;

  case DISKDEV:
    dev = new DiskDevice(this, config, intl, dnum);
    break;

  case FLASHDEV:
    dev = new FlashDevice(this, config, intl, dnum);
    break;

  default:
    dev = new Device(this, intl, dnum);
    break;
  }

  return dev;
}

// This method writes the data at the physical address addr, and passes it
// back thru the datap pointer. It also return FALSE if the addr is valid
// and writable, and TRUE otherwise
bool SystemBus::busWrite(Word addr, Word data, Processor *cpu) {
  if (INBOUNDS(addr, RAMBASE, RAMBASE + ram->Size())) {
    ram->MemWrite(CONVERT(addr, RAMBASE), data);
  } else if (INBOUNDS(addr, BIOSDATABASE, BIOSDATABASE + biosdata->Size())) {
    biosdata->MemWrite(CONVERT(addr, BIOSDATABASE), data);
  } else if (INBOUNDS(addr, MMIO_BASE, MMIO_END)) {
    if (DEV_REG_START <= addr && addr < DEV_REG_END) {
      DeviceAreaAddress dva(addr);
      Device *device = devTable[dva.line()][dva.device()];
      device->WriteDevReg(dva.field(), data);
    } else if (INBOUNDS(addr, IRT_BASE, IRT_END) ||
               INBOUNDS(addr, CPUCTL_BASE, CPUCTL_END)) {
      pic->Write(addr, data, cpu);
    } else if (MCTL_BASE <= addr && addr < MCTL_END) {
      mpController->Write(addr, data, NULL);
    } else {
      // data write is in bus registers area
      if (addr == BUS_REG_TIMER) {
        // update the interval timer and reset its interrupt line
        timer = data;
        pic->EndIRQ(IL_TIMER);
      }
      // else data write is on a read only bus register, and
      // has no harmful effects
    }
  } else {
    // Address out of valid write bounds
    return (true);
  }

  return (false);
}
