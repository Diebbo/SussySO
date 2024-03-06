/*
 * uRISCV - A general purpose computer system simulator
 *
 * Copyright (C) 2011 Tomislav Jonjic
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

#ifndef URISCV_MPIC_H
#define URISCV_MPIC_H

#include <deque>
#include <vector>

#include "uriscv/machine_config.h"

class SystemBus;
class Processor;

class InterruptController {
public:
  static const Word kIpiLatency = 20;

  InterruptController(const MachineConfig *config, SystemBus *bus);

  void StartIRQ(unsigned int il, unsigned int devNo = 0);
  void EndIRQ(unsigned int il, unsigned int devNo = 0);

  Word Read(Word addr, const Processor *cpu) const;
  void Write(Word addr, Word data, const Processor *cpu);

  Word GetIP(Word cpuId) const { return cpuData[cpuId].ipMask << (0); }

private:
  static const unsigned int kBaseIL = 16;
  static const unsigned int kSharedILBase = 1;

  static const Word kInvalidCpuId = ~0U;

  struct Source {
    Source() : lastTarget(kInvalidCpuId) {
      route.destination = 0;
      route.policy = IRT_POLICY_FIXED;
    }

    // Core the last interrupt from this source was delivered to, needed
    // for ack messages. We have to keep this because the routing info can
    // change at any time.
    Word lastTarget;

    // IRT entry fields
    struct {
      unsigned destination : MachineConfig::MAX_CPUS;
      unsigned policy : 1;
    } route;
  };

  struct IpiMessage {
    unsigned origin : 4;
    unsigned msg : 8;
  };

  struct CpuData {
    CpuData() {
      ipMask = 0;
      for (Word &data : idb)
        data = 0;
      taskPriority = CPUCTL_TPR_PRIORITY_MASK;
    }

    Word ipMask;
    Word idb[N_EXT_IL];
    std::deque<IpiMessage> ipiInbox;
    unsigned int taskPriority;
    Word biosReserved[2];
  };

  void deliverIPI(unsigned int origin, Word outbox);

  const MachineConfig *const config;
  SystemBus *const bus;

  // Simple rotating index used to break ties between cpu
  // destinations with equal task priorities
  unsigned int arbiter;

  // Incoming int. sources (first N_EXT_IL are devices, the last one is TIMER)
  Source sources[N_EXT_IL + 1][N_DEV_PER_IL];

  // Int. controller cpu interface, for each core
  std::vector<CpuData> cpuData;
};

#endif // URISCV_MPIC_H
