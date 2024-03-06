/*
 * uRISCV - A general purpose computer system simulator
 *
 * Copyright (C) 2010, 2011 Tomislav Jonjic
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

#include "qriscv/debug_session.h"

#include <algorithm>
#include <cstdlib>
#include <list>

#include <QAction>
#include <QMessageBox>
#include <QTimer>

#include "qriscv/application.h"
#include "uriscv/error.h"

const unsigned int DebugSession::kIterCycles[kNumSpeedLevels] = {5, 25, 250,
                                                                 2500, 20000};

const unsigned int DebugSession::kIterInterval[kNumSpeedLevels] = {50, 25, 5, 1,
                                                                   0};

DebugSession::DebugSession() : status(MS_HALTED), idleSteps(0) {
  createActions();
  updateActionSensitivity();
  connect(this, SIGNAL(StatusChanged()), this, SLOT(updateActionSensitivity()));
  connect(Appl(), SIGNAL(MachineConfigChanged()), this,
          SLOT(onMachineConfigChanged()));

  timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(runIteration()));

  idleTimer = new QTimer(this);
  connect(idleTimer, SIGNAL(timeout()), this, SLOT(skip()));

  setSpeed(Appl()->settings.value("SimulationSpeed", kMaxSpeed).toInt());
  stopMask = Appl()->settings.value("StopMask", kDefaultStopMask).toUInt();
}

void DebugSession::halt() {
  if (!isStarted())
    return;

  timer->stop();
  idleTimer->stop();

  machine.reset();
  bplModel.reset();

  Q_EMIT MachineHalted();
  setStatus(MS_HALTED);
}

void DebugSession::setSpeed(int value) {
  value = qBound(0, value, kMaxSpeed);
  if (speed != value) {
    speed = value;
    Appl()->settings.setValue("SimulationSpeed", speed);
    timer->setInterval(kIterInterval[speed]);
    Q_EMIT SpeedChanged(speed);
  }
}

void DebugSession::setStopMask(unsigned int value) {
  stopMask = value;
  Appl()->settings.setValue("StopMask", stopMask);

  if (machine.get())
    machine->setStopMask(stopMask);
}

void DebugSession::createActions() {
  startMachineAction = new QAction("Power On", this);
  startMachineAction->setShortcut(QKeySequence("F5"));
  startMachineAction->setIcon(QIcon(":/icons/start-22.svg"));
  connect(startMachineAction, SIGNAL(triggered()), this, SLOT(startMachine()));
  startMachineAction->setEnabled(false);

  haltMachineAction = new QAction("Power Off", this);
  haltMachineAction->setShortcut(QKeySequence("Shift+F5"));
  haltMachineAction->setIcon(QIcon(":/icons/halt-22.svg"));
  connect(haltMachineAction, SIGNAL(triggered()), this, SLOT(onHaltMachine()));
  haltMachineAction->setEnabled(false);

  toggleMachineAction = new QAction("Power On", this);
  toggleMachineAction->setIcon(QIcon(":/icons/start-22.svg"));
  connect(toggleMachineAction, SIGNAL(triggered()), this,
          SLOT(toggleMachine()));
  toggleMachineAction->setEnabled(false);

  resetMachineAction = new QAction("Reset", this);
  resetMachineAction->setShortcut(QKeySequence("F6"));
  resetMachineAction->setIcon(QIcon(":/icons/reset-22.svg"));
  connect(resetMachineAction, SIGNAL(triggered()), this,
          SLOT(onResetMachine()));
  resetMachineAction->setEnabled(false);

  debugContinueAction = new QAction("&Continue", this);
  debugContinueAction->setShortcut(QKeySequence("F9"));
  debugContinueAction->setIcon(QIcon(":/icons/continue-22.svg"));
  connect(debugContinueAction, SIGNAL(triggered()), this, SLOT(onContinue()));

  debugStepAction = new QAction("Step", this);
  debugStepAction->setShortcut(QKeySequence("F10"));
  debugStepAction->setIcon(QIcon(":/icons/step-22.svg"));
  connect(debugStepAction, SIGNAL(triggered()), this, SLOT(onStep()));

  debugStopAction = new QAction("&Stop", this);
  debugStopAction->setShortcut(QKeySequence("F12"));
  debugStopAction->setIcon(QIcon(":/icons/stop-22.svg"));
  connect(debugStopAction, SIGNAL(triggered()), this, SLOT(stop()));

  debugToggleAction = new QAction("Continue", this);
  debugToggleAction->setIcon(QIcon(":/icons/continue-22.svg"));
  connect(debugToggleAction, SIGNAL(triggered()), this, SLOT(toggleDebug()));
}

void DebugSession::updateActionSensitivity() {
  bool started = (status != MS_HALTED);
  bool stopped = (status == MS_STOPPED);
  bool running = (status == MS_RUNNING);

  startMachineAction->setEnabled(Appl()->getConfig() != NULL && !started);
  haltMachineAction->setEnabled(started);
  toggleMachineAction->setEnabled(Appl()->getConfig() != NULL);
  resetMachineAction->setEnabled(started);

  debugContinueAction->setEnabled(stopped);
  debugStepAction->setEnabled(stopped);
  debugStopAction->setEnabled(running);
  debugToggleAction->setEnabled(Appl()->getConfig() != NULL && started);

  if (startMachineAction->isEnabled() || Appl()->getConfig() == NULL) {
    toggleMachineAction->setToolTip("Power On");
    toggleMachineAction->setIcon(QIcon(":/icons/start-22.svg"));
  } else {
    toggleMachineAction->setToolTip("Power Off");
    toggleMachineAction->setIcon(QIcon(":/icons/halt-22.svg"));
  }

  if (debugStopAction->isEnabled()) {
    debugToggleAction->setToolTip("Stop");
    debugToggleAction->setIcon(QIcon(":/icons/stop-22.svg"));
  } else {
    debugToggleAction->setToolTip("Continue");
    debugToggleAction->setIcon(QIcon(":/icons/continue-22.svg"));
  }
}

void DebugSession::setStatus(MachineStatus newStatus) {
  if (newStatus != status) {
    status = newStatus;
    Q_EMIT StatusChanged();
  }
}

void DebugSession::initializeMachine() {
  MachineConfig *config = Appl()->getConfig();
  assert(config != NULL);

  std::list<std::string> errors;
  if (!config->Validate(&errors)) {
    QString el;
    el += "<ul>";
    for (const std::string &s : errors)
      el += QString("<li>%1</li>").arg(s.c_str());
    el += "</ul>";
    QMessageBox::critical(
        0, QString("%1: Error").arg(Appl()->applicationName()),
        "Invalid and/or incomplete machine configuration: " + el);
    return;
  }

  try {
    machine.reset(new Machine(config, &breakpoints, &suspects, &tracepoints));
  } catch (const FileError &e) {
    QMessageBox::critical(
        Appl()->getApplWindow(),
        QString("%1: Error").arg(Appl()->applicationName()),
        QString("<b>Could not initialize machine:</b> "
                "the file `%1' is nonexistent or inaccessible")
            .arg(e.fileName.c_str()));
    return;
  } catch (const InvalidCoreFileError &e) {
    QMessageBox::critical(
        Appl()->getApplWindow(),
        QString("%1: Error").arg(Appl()->applicationName()),
        QString("<b>Could not initialize machine:</b> "
                "the file `%1' does not appear to be a valid <i>Core</i> file; "
                "make sure you are creating the file with the "
                "<code>umps3-elf2umps</code> utility")
            .arg(e.fileName.c_str()));
    return;
  } catch (const CoreFileOverflow &e) {
    QMessageBox::critical(Appl()->getApplWindow(),
                          QString("%1: Error").arg(Appl()->applicationName()),
                          "<b>Could not initialize machine:</b> "
                          "the core file does not fit in memory; "
                          "please increase available RAM and try again");
    return;
  } catch (const InvalidFileFormatError &e) {
    QMessageBox::critical(Appl()->getApplWindow(),
                          QString("%1: Error").arg(Appl()->applicationName()),
                          QString("<b>Could not initialize machine:</b> "
                                  "the file `%1' has wrong format")
                              .arg(e.fileName.c_str()));
    return;
  } catch (const EthError &e) {
    QMessageBox::critical(Appl()->getApplWindow(),
                          QString("%1: Error").arg(Appl()->applicationName()),
                          QString("<b>Could not initialize machine:</b> "
                                  "error initializing network device %1")
                              .arg(e.devNo));
    return;
  }

  machine->setStopMask(stopMask);

  SymbolTable *stab;
  try {
    stab = new SymbolTable(config->getSymbolTableASID(),
                           config->getROM(ROM_TYPE_STAB).c_str());
  } catch (const Error &e) {
    QMessageBox::critical(Appl()->getApplWindow(),
                          QString("%1: Error").arg(Appl()->applicationName()),
                          "<b>Could not initialize machine:</b> "
                          "invalid or missing symbol table file");
    machine.reset();
    return;
  }

  if (symbolTable && stab->getASID() == symbolTable->getASID()) {
    relocateStoppoints(stab, breakpoints);
    relocateStoppoints(stab, suspects);
    relocateStoppoints(stab, tracepoints);
  }
  symbolTable.reset(stab);

  bplModel.reset(new StoppointListModel(&breakpoints, "Breakpoint", 'B'));

  stoppedByUser = true;
  setStatus(MS_STOPPED);

  cpuStatusMap.reset(new CpuStatusMap(this));
}

void DebugSession::onMachineConfigChanged() {
  if (Appl()->getConfig() != NULL) {
    startMachineAction->setEnabled(true);
    toggleMachineAction->setEnabled(true);
  }

  breakpoints.Clear();
  suspects.Clear();
  tracepoints.Clear();
}

void DebugSession::startMachine() {
  assert(status == MS_HALTED);

  initializeMachine();
  if (machine)
    Q_EMIT MachineStarted();
}

void DebugSession::onHaltMachine() {
  assert(status != MS_HALTED);
  halt();
}

void DebugSession::toggleMachine() {
  if (startMachineAction->isEnabled())
    startMachine();
  else
    onHaltMachine();
}

void DebugSession::onResetMachine() {
  assert(isStarted());

  stop();

  machine.reset();
  initializeMachine();
  if (machine) {
    Q_EMIT MachineReset();
  } else {
    Q_EMIT MachineHalted();
    setStatus(MS_HALTED);
  }
}

void DebugSession::onContinue() {
  assert(status == MS_STOPPED);

  stepping = false;
  stoppedByUser = false;
  Q_EMIT MachineRan();
  setStatus(MS_RUNNING);

  timer->start();
}

void DebugSession::onStep() { step(1); }

void DebugSession::stop() {
  if (isRunning()) {
    stoppedByUser = true;
    timer->stop();
    idleTimer->stop();
    setStatus(MS_STOPPED);
    Q_EMIT MachineStopped();
  }
}

void DebugSession::toggleDebug() {
  if (debugContinueAction->isEnabled())
    onContinue();
  else
    stop();
}

void DebugSession::step(unsigned int steps) {
  assert(status == MS_STOPPED);

  stepping = true;
  stepsLeft = steps;
  stoppedByUser = false;
  Q_EMIT MachineRan();
  setStatus(MS_RUNNING);

  // Always step through at least one cycle (might be a bit too
  // pedantic but oh well...)
  bool stopped;
  machine->step(&stopped);
  --stepsLeft;

  if (machine->IsHalted()) {
    halt();
  } else if (!stepsLeft || stopped) {
    stoppedByUser = !stepsLeft;
    setStatus(MS_STOPPED);
    Q_EMIT MachineStopped();
  } else {
    timer->start();
  }
}

void DebugSession::runIteration() {
  if (stepping)
    runStepIteration();
  else
    runContIteration();
}

void DebugSession::runStepIteration() {
  unsigned int steps = std::min(stepsLeft, kIterCycles[speed]);

  bool stopped = false;
  unsigned int stepped;
  machine->step(steps, &stepped, &stopped);
  stepsLeft -= stepped;

  if (machine->IsHalted()) {
    halt();
  } else if (stopped || stepsLeft == 0) {
    stoppedByUser = (stepsLeft == 0);
    timer->stop();
    setStatus(MS_STOPPED);
    Q_EMIT MachineStopped();
  } else {
    Q_EMIT DebugIterationCompleted();
  }
}

void DebugSession::runContIteration() {
  idleSteps = machine->idleCycles();
  if (idleSteps > 0) {
    // Enter low-power mode!
    timer->stop();
    const qreal ticksPerMsec =
        1000.0 * qreal(Appl()->getConfig()->getClockRate());
    const int interval =
        qRound(qreal(std::min(idleSteps, kMaxSkipped)) / ticksPerMsec);
    idleTimer->start(interval);
  } else {
    bool stopped;
    machine->step(kIterCycles[speed], NULL, &stopped);
    if (machine->IsHalted()) {
      halt();
    } else if (stopped) {
      setStatus(MS_STOPPED);
      Q_EMIT MachineStopped();
      timer->stop();
    } else {
      Q_EMIT DebugIterationCompleted();
    }
  }
}

void DebugSession::skip() {
  assert(idleSteps > 0);

  const uint32_t skipped = std::min(idleSteps, kMaxSkipped);
  machine->skip(skipped);
  idleSteps -= skipped;

  // Keep skipping cycles while the machine is idle.
  if (idleSteps == 0) {
    idleTimer->stop();
    timer->start();
  } else if (idleSteps < kMaxSkipped) {
    const qreal ticksPerMsec =
        1000.0 * qreal(Appl()->getConfig()->getClockRate());
    idleTimer->setInterval(qRound(qreal(idleSteps) / ticksPerMsec));
  }

  Q_EMIT DebugIterationCompleted();
}

void DebugSession::relocateStoppoints(const SymbolTable *newTable,
                                      StoppointSet &set) {
  StoppointSet rset;

  for (Stoppoint::Ptr sp : set) {
    const AddressRange &origin = sp->getRange();
    const Symbol *symbol =
        symbolTable->Probe(origin.getASID(), origin.getStart(), true);
    if (symbol != NULL) {
      std::list<const Symbol *> dest =
          newTable->Lookup(symbol->getName(), symbol->getType());
      if (dest.size() == 1) {
        Word start =
            dest.front()->getStart() + symbol->Offset(origin.getStart());
        Word end = start + (origin.getEnd() - origin.getStart());
        rset.Add(AddressRange(origin.getASID(), start, end),
                 sp->getAccessMode(), sp->getId(), sp->IsEnabled());
        continue;
      }
    }
    rset.Add(origin, sp->getAccessMode(), sp->getId(), sp->IsEnabled());
  }

  set = rset;
}
