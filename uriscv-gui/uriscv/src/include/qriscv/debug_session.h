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

#ifndef QRISCV_DEBUG_SESSION_H
#define QRISCV_DEBUG_SESSION_H

#include <QObject>

#include "base/lang.h"
#include "qriscv/cpu_status_map.h"
#include "qriscv/stoppoint_list_model.h"
#include "uriscv/machine.h"
#include "uriscv/stoppoint.h"
#include "uriscv/symbol_table.h"

enum MachineStatus { MS_HALTED, MS_RUNNING, MS_STOPPED };

class QAction;
class Machine;
class QTimer;

class DebugSession : public QObject {
  Q_OBJECT

public:
  static const int kNumSpeedLevels = 5;
  static const int kMaxSpeed = kNumSpeedLevels - 1;

  static const int kDefaultStopMask =
      (SC_BREAKPOINT | SC_SUSPECT | SC_EXCEPTION);

  DebugSession();

  MachineStatus getStatus() const { return status; }

  bool isStopped() const { return status == MS_STOPPED; }
  bool isStoppedByUser() const { return stoppedByUser; }
  bool isRunning() const { return status == MS_RUNNING; }
  bool isStarted() const { return status != MS_HALTED; }

  void halt();

  unsigned int getStopMask() const { return stopMask; }
  int getSpeed() const { return speed; }

  Machine *getMachine() const { return machine.get(); }
  SymbolTable *getSymbolTable() { return symbolTable.get(); }

  StoppointSet *getBreakpoints() { return &breakpoints; }
  StoppointListModel *getBreakpointListModel() { return bplModel.get(); }

  StoppointSet *getSuspects() { return &suspects; }
  StoppointSet *getTracepoints() { return &tracepoints; }

  const CpuStatusMap *getCpuStatusMap() const { return cpuStatusMap.get(); }

  // Global actions
  QAction *startMachineAction;
  QAction *haltMachineAction;
  QAction *toggleMachineAction;
  QAction *resetMachineAction;

  QAction *debugContinueAction;
  QAction *debugStepAction;
  QAction *debugStopAction;
  QAction *debugToggleAction;

public Q_SLOTS:
  void setStopMask(unsigned int value);
  void setSpeed(int value);
  void stop();

Q_SIGNALS:
  void StatusChanged();
  void MachineStarted();
  void MachineStopped();
  void MachineRan();
  void MachineHalted();
  void MachineReset();
  void DebugIterationCompleted();

  void SpeedChanged(int);

private:
  static const uint32_t kMaxSkipped = 50000;

  void createActions();
  void setStatus(MachineStatus newStatus);

  void initializeMachine();

  void step(unsigned int steps);
  void runStepIteration();
  void runContIteration();

  void relocateStoppoints(const SymbolTable *newTable, StoppointSet &set);

  MachineStatus status;
  scoped_ptr<Machine> machine;

  scoped_ptr<SymbolTable> symbolTable;

  // We need a "proxy" stop mask here since it has to live through
  // machine reconfigurations, resets, etc.
  unsigned int stopMask;

  int speed;
  static const unsigned int kIterCycles[kNumSpeedLevels];
  static const unsigned int kIterInterval[kNumSpeedLevels];

  StoppointSet breakpoints;
  scoped_ptr<StoppointListModel> bplModel;
  StoppointSet suspects;
  StoppointSet tracepoints;

  scoped_ptr<CpuStatusMap> cpuStatusMap;

  bool stoppedByUser;

  bool stepping;
  unsigned int stepsLeft;

  QTimer *timer;
  QTimer *idleTimer;

  uint32_t idleSteps;

private Q_SLOTS:
  void onMachineConfigChanged();

  void startMachine();
  void onHaltMachine();
  void toggleMachine();
  void onResetMachine();
  void onContinue();
  void onStep();
  void toggleDebug();

  void updateActionSensitivity();

  void runIteration();
  void skip();
};

#endif // QRISCV_DEBUG_SESSION_H
