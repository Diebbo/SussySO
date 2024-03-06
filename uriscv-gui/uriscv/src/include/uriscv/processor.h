/*
 * uRISCV - A general purpose computer system simulator
  scoped_array<TLBEntry> tlb;
 *
 * Copyright (C) 2004 Mauro Morsiani
 * Copyright (C) 2020 Mattia Biondi
 * Copyright (C) 2023 Gianmaria Rovelli
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

#ifndef URISCV_PROCESSOR_H
#define URISCV_PROCESSOR_H

#include <sigc++/sigc++.h>
#include <string>

#include "base/lang.h"
#include "uriscv/const.h"
#include "uriscv/types.h"

class MachineConfig;
class Machine;
class SystemBus;
class TLBEntry;

enum ProcessorStatus { PS_HALTED, PS_RUNNING, PS_IDLE };

class Processor {
public:
  // Register file size:
  static const unsigned int kNumCPURegisters = 32;
  static const unsigned int kNumCSRRegisters = 4096;

  Processor(const MachineConfig *config, Word id, Machine *machine,
            SystemBus *bus);
  virtual ~Processor();

  Word getId() const { return id; }
  Word Id() const { return id; }

  ProcessorStatus getStatus() const { return status; }

  bool isHalted() const { return status == PS_HALTED; }
  bool isRunning() const { return status == PS_RUNNING; }
  bool isIdle() const { return status == PS_IDLE; }

  void Reset(Word pc, Word sp);
  void Halt();

  // This method makes Processor execute a single instruction.  For
  // simulation purposes, it differs from traditional processor cycle:
  // the first instruction after a reset is pre-loaded, and cycle is
  // execute - fetch instead of fetch - execute.  This way, it is
  // possible to know what instruction will be executed before its
  // execution happens
  void Cycle();

  uint32_t IdleCycles();

  void Skip(uint32_t cycles);

  // This method allows SystemBus and Processor itself to signal
  // Processor when an exception happens. SystemBus signal IBE/DBE
  // exceptions; Processor itself signal all other kinds of exception.
  void SignalExc(unsigned int exc, Word cpuNum = 0UL);

  void AssertIRQ(unsigned int il);
  void DeassertIRQ(unsigned int il);

  // The following methods allow inspection of Processor internal
  // status. Name & parameters are self-explanatory: remember that
  // all addresses are _virtual_ when not marked Phys/P/phys (for
  // physical ones), and all index checking must be performed from
  // caller.

  void getCurrStatus(Word *asid, Word *pc, Word *instr, bool *isLD, bool *isBD);

  Word getASID();
  Word getPC() const { return currPC; }
  Word getInstruction() const { return currInstr; }

  bool InUserMode();
  bool InKernelMode();

  void getPrevStatus(Word *pc, Word *instr);
  const char *getExcCauseStr();
  Word incrementPC(Word value);
  Word getNextPC(void);
  Word getSuccPC(void);
  Word getPrevPPC(void);
  Word getCurrPPC(void);
  SWord getGPR(unsigned int num);
  Word getCP0Reg(unsigned int num);
  void getTLB(unsigned int index, Word *hi, Word *lo) const;
  Word getTLBHi(unsigned int index) const;
  Word getTLBLo(unsigned int index) const;

  Word regRead(Word reg);
  void regWrite(Word reg, Word value);
  Word csrRead(Word reg);
  void csrWrite(Word reg, Word value);

  // The following methods allow to change Processor internal status
  // Name & parameters are almost self-explanatory: remember that
  // all addresses are _virtual_ when not marked Phys/P/phys (for
  // physical ones), and all index checking must be performed from
  // caller.  Processor status modification is allowed during
  // debugging inside the simulation.

  void setGPR(unsigned int num, SWord val);
  void setPC(Word npc);
  void setNextPC(Word npc);
  void setSuccPC(Word spc);
  void setTLB(unsigned int index, Word hi, Word lo);
  void setTLBHi(unsigned int index, Word value);
  void setTLBLo(unsigned int index, Word value);

  // Signals
  sigc::signal<void> StatusChanged;
  sigc::signal<void, unsigned int> SignalException;
  sigc::signal<void, unsigned int> SignalTLBChanged;

private:
  enum MultiplierPorts { HI = 32, LO = 33 };

  // Type of destination register for a pending load
  enum LoadTargetType {
    LOAD_TARGET_GPREG,
    LOAD_TARGET_CPREG,
    LOAD_TARGET_NONE
  };

  const Word id;

  // object references for memory access (bus) and for virtual address
  // accessing (watch)
  const MachineConfig *config;
  Machine *machine;
  SystemBus *bus;

  std::string prevFunc;
  bool skipCycle;

  // 3 - M ... 0 - U
  unsigned int mode;
  ProcessorStatus status;

  // last exception cause: an internal format is used (see excName[]
  // for mnemonic code) and it is mapped to CAUSE register format by
  // excCode[] array
  unsigned int excCause;

  // for CPUEXCEPTION, coprocessor unusable number
  Word copENum;

  // tracks branch delay slots
  bool isBranchD;

  // delayed load handling variables:
  // indicates if a delayed load is pending
  LoadTargetType loadPending;
  // register target
  unsigned int loadReg;
  // value to be loaded into register
  SWord loadVal;

  // general purpose registers, together with HI and LO registers
  SWord gpr[kNumCPURegisters];
  csr_t csr[kNumCSRRegisters];

  // instruction to be executed
  Word currInstr;

  // previous virtual and physical addresses for PC, and previous
  // instruction executed; for book-keeping purposes and for handling
  // exceptions in BD slot
  Word prevPC;
  Word prevPhysPC;
  Word prevInstr;

  // current virtual and physical addresses for PC
  Word currPC;
  Word currPhysPC;

  // virtual values for PC after current one: they are needed to
  // emulate branch delay slot; no physical values are available since
  // conversion is needed (and sometimes possible) only for PC current
  // value
  Word nextPC;
  Word succPC;

  // CP0 components: special registers and the TLB
  // Word cpreg[CP0REGNUM];

  size_t tlbSize;
  scoped_array<TLBEntry> tlb;

  Word tlbFloorAddress;

  void initCSR();

  // private methods
  void setStatus(ProcessorStatus newStatus);

  void handleExc();
  void zapTLB(void);

  bool execInstr(Word instr);
  bool execInstrL(Word instr);
  bool execInstrR(Word instr);
  bool execInstrI(Word instr);
  bool execInstrI2(Word instr);
  bool execInstrB(Word instr);
  bool execInstrS(Word instr);

  bool mapVirtual(Word vaddr, Word *paddr, Word accType);
  bool probeTLB(unsigned int *index, Word asid, Word vpn);
  void completeLoad(void);

  void randomRegTick(void);

  void pushKUIEStack(void);
  void popKUIEStack(void);

  void setTLBRegs(Word vaddr);
  bool checkForInt();
  void suspend();
  void setLoad(LoadTargetType loadCode, unsigned int regNum, SWord regVal);
  SWord signExtByte(Word val, unsigned int bytep);
  Word zExtByte(Word val, unsigned int bytep);
  SWord signExtHWord(Word val, unsigned int hwp);
  Word zExtHWord(Word val, unsigned int hwp);
  Word mergeByte(Word dest, Word src, unsigned int bytep);
  Word mergeHWord(Word dest, Word src, unsigned int hwp);
  Word merge(Word dest, Word src, unsigned int bytep, bool loadBig,
             bool startLeft);

  DISABLE_COPY_AND_ASSIGNMENT(Processor);
};

#endif // URISCV_PROCESSOR_H
