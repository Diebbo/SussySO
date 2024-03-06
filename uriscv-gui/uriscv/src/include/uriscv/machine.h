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

#ifndef URISCV_MACHINE_H
#define URISCV_MACHINE_H

#include <vector>

#include "base/lang.h"
#include "uriscv/device.h"
#include "uriscv/machine_config.h"
#include "uriscv/stoppoint.h"
#include "uriscv/symbol_table.h"
#include "uriscv/systembus.h"

enum StopCause {
  SC_USER = 1 << 0,
  SC_BREAKPOINT = 1 << 1,
  SC_SUSPECT = 1 << 2,
  SC_EXCEPTION = 1 << 3,
  SC_UTLB_KERNEL = 1 << 4,
  SC_UTLB_USER = 1 << 5
};

class Processor;
class SystemBus;
class Device;
class StoppointSet;

class Machine {
public:
  Machine(const MachineConfig *config, StoppointSet *breakpoints,
          StoppointSet *suspects, StoppointSet *tracepoints);
  ~Machine();

  void step(bool *stopped = NULL);
  void step(unsigned int steps, unsigned int *stepped = NULL,
            bool *stopped = NULL);

  uint32_t idleCycles() const;
  void skip(uint32_t cycles);

  void Halt();
  bool IsHalted() const { return halted; }

  Processor *getProcessor(unsigned int cpuId);
  Device *getDevice(unsigned int line, unsigned int devNo);
  SystemBus *getBus();

  void setStopMask(unsigned int mask);
  unsigned int getStopMask() const;

  unsigned int getStopCause(unsigned int cpuId) const;
  unsigned int getActiveBreakpoint(unsigned int cpuId) const;
  unsigned int getActiveSuspect(unsigned int cpuId) const;

  bool ReadMemory(Word physAddr, Word *data);
  bool WriteMemory(Word paddr, Word data);

  void HandleBusAccess(Word pAddr, Word access, Processor *cpu);
  void HandleVMAccess(Word asid, Word vaddr, Word access, Processor *cpu);

  void setStab(SymbolTable *stab);
  SymbolTable *getStab();

private:
  struct ProcessorData {
    unsigned int stopCause;
    unsigned int breakpointId;
    unsigned int suspectId;
  };

  void onCpuStatusChanged(const Processor *cpu);
  void onCpuException(unsigned int, Processor *cpu);

  unsigned int stopMask;

  const MachineConfig *const config;

  scoped_ptr<SystemBus> bus;

  typedef std::vector<Processor *> CpuVector;
  std::vector<Processor *> cpus;

  ProcessorData pd[MachineConfig::MAX_CPUS];

  bool halted;
  bool stopRequested;
  bool pauseRequested;

  StoppointSet *breakpoints;
  StoppointSet *suspects;
  StoppointSet *tracepoints;

  SymbolTable *stab;
};

#endif // URISCV_MACHINE_H
