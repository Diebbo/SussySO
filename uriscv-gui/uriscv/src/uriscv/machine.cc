/*
 * uRISCV - A general purpose computer system simulator
 *
 * Copyright (C) 2010 Tomislav Jonjic
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

#include "uriscv/machine.h"

#include <cassert>
#include <cstdlib>

#include "base/lang.h"

#include "uriscv/const.h"
#include "uriscv/machine_config.h"
#include "uriscv/processor.h"
#include "uriscv/stoppoint.h"
#include "uriscv/symbol_table.h"
#include "uriscv/systembus.h"
#include "uriscv/types.h"

Machine::Machine(const MachineConfig *config, StoppointSet *breakpoints,
                 StoppointSet *suspects, StoppointSet *tracepoints)
    : stopMask(0), config(config), halted(false), breakpoints(breakpoints),
      suspects(suspects), tracepoints(tracepoints) {
  assert(config->Validate(NULL));

  bus.reset(new SystemBus(config, this));

  for (unsigned int i = 0; i < config->getNumProcessors(); i++) {
    Processor *cpu = new Processor(config, i, this, bus.get());
    cpu->SignalException.connect(
        sigc::bind(sigc::mem_fun(this, &Machine::onCpuException), cpu));
    cpu->StatusChanged.connect(
        sigc::bind(sigc::mem_fun(this, &Machine::onCpuStatusChanged), cpu));
    pd[i].stopCause = 0;
    cpus.push_back(cpu);
  }

  cpus[0]->Reset(MCTL_DEFAULT_BOOT_PC, MCTL_DEFAULT_BOOT_SP);
}

Machine::~Machine() {
  for (Processor *p : cpus)
    delete p;
}

void Machine::step(unsigned int steps, unsigned int *stepped, bool *stopped) {
  stopRequested = pauseRequested = false;
  for (Processor *cpu : cpus)
    pd[cpu->Id()].stopCause = 0;

  unsigned int i;
  for (i = 0; !halted && i < steps && !stopRequested && !pauseRequested; ++i) {
    bus->ClockTick();
    for (CpuVector::iterator it = cpus.begin(); it != cpus.end(); ++it)
      (*it)->Cycle();
  }
  if (stepped)
    *stepped = i;
  if (stopped)
    *stopped = stopRequested;
}

void Machine::step(bool *stopped) { step(1, NULL, stopped); }

uint32_t Machine::idleCycles() const {
  uint32_t c;

  if ((c = bus->IdleCycles()) == 0)
    return 0;

  for (Processor *cpu : cpus) {
    c = std::min(c, cpu->IdleCycles());
    if (c == 0)
      return 0;
  }

  return c;
}

void Machine::skip(uint32_t cycles) {
  bus->Skip(cycles);
  for (Processor *cpu : cpus) {
    if (!cpu->isHalted())
      cpu->Skip(cycles);
  }
}

void Machine::Halt() { halted = true; }

void Machine::onCpuException(unsigned int excCode, Processor *cpu) {
  bool utlbExc = (excCode == UTLBLEXCEPTION || excCode == UTLBSEXCEPTION);

  if (((stopMask & SC_EXCEPTION) && !utlbExc) ||
      ((stopMask & SC_UTLB_USER) && utlbExc && cpu->InUserMode()) ||
      ((stopMask & SC_UTLB_KERNEL) && utlbExc && cpu->InKernelMode())) {
    pd[cpu->getId()].stopCause |= SC_EXCEPTION;
    stopRequested = true;
  }
}

void Machine::onCpuStatusChanged(const Processor *cpu) {
  // Whenever a cpu goes to sleep, give the client a chance to
  // detect idle machine states.
  if (cpu->isIdle())
    pauseRequested = true;
}

void Machine::HandleBusAccess(Word pAddr, Word access, Processor *cpu) {
  // Check for breakpoints and suspects
  switch (access) {
  case READ:
  case WRITE:
    if (stopMask & SC_SUSPECT && suspects != NULL) {
      Stoppoint *suspect = suspects->Probe(
          MAXASID, pAddr, (access == READ) ? AM_READ : AM_WRITE, cpu);
      if (suspect != NULL) {
        pd[cpu->getId()].stopCause |= SC_SUSPECT;
        pd[cpu->getId()].suspectId = suspect->getId();
        stopRequested = true;
      }
    }
    break;

  case EXEC:
    if (stopMask & SC_BREAKPOINT && breakpoints != NULL) {
      Stoppoint *breakpoint = breakpoints->Probe(MAXASID, pAddr, AM_EXEC, cpu);
      if (breakpoint != NULL) {
        pd[cpu->getId()].stopCause |= SC_BREAKPOINT;
        pd[cpu->getId()].breakpointId = breakpoint->getId();
        stopRequested = true;
      }
    }
    break;

  default:
    // Assert not reached
    assert(0);
  }

  // Check for traced ranges
  if (access == WRITE && tracepoints != NULL) {
    Stoppoint *tracepoint = tracepoints->Probe(MAXASID, pAddr, AM_WRITE, cpu);
    (void)tracepoint;
  }
}

void Machine::HandleVMAccess(Word asid, Word vaddr, Word access,
                             Processor *cpu) {
  switch (access) {
  case READ:
  case WRITE:
    if (stopMask & SC_SUSPECT && suspects != NULL) {
      Stoppoint *suspect = suspects->Probe(
          asid, vaddr, (access == READ) ? AM_READ : AM_WRITE, cpu);
      if (suspect != NULL) {
        pd[cpu->Id()].stopCause |= SC_SUSPECT;
        pd[cpu->Id()].suspectId = suspect->getId();
        stopRequested = true;
      }
    }
    break;

  case EXEC:
    if (stopMask & SC_BREAKPOINT && breakpoints != NULL) {
      Stoppoint *breakpoint = breakpoints->Probe(asid, vaddr, AM_EXEC, cpu);
      if (breakpoint != NULL) {
        pd[cpu->Id()].stopCause |= SC_BREAKPOINT;
        pd[cpu->Id()].breakpointId = breakpoint->getId();
        stopRequested = true;
      }
    }
    break;

  default:
    // Assert not reached
    assert(0);
  }
}

Processor *Machine::getProcessor(unsigned int cpuId) { return cpus[cpuId]; }

Device *Machine::getDevice(unsigned int line, unsigned int devNo) {
  return bus->getDev(line, devNo);
}

SystemBus *Machine::getBus() { return bus.get(); }

void Machine::setStopMask(unsigned int mask) { stopMask = mask; }

unsigned int Machine::getStopMask() const { return stopMask; }

unsigned int Machine::getStopCause(unsigned int cpuId) const {
  assert(cpuId < config->getNumProcessors());
  return pd[cpuId].stopCause;
}

unsigned int Machine::getActiveBreakpoint(unsigned int cpuId) const {
  assert(cpuId < config->getNumProcessors());
  return pd[cpuId].breakpointId;
}

unsigned int Machine::getActiveSuspect(unsigned int cpuId) const {
  assert(cpuId < config->getNumProcessors());
  return pd[cpuId].suspectId;
}

bool Machine::ReadMemory(Word physAddr, Word *data) {
  return bus->WatchRead(physAddr, data);
}

bool Machine::WriteMemory(Word paddr, Word data) {
  return bus->WatchWrite(paddr, data);
}

void Machine::setStab(SymbolTable *stab) { this->stab = stab; }

SymbolTable *Machine::getStab() { return stab; }
