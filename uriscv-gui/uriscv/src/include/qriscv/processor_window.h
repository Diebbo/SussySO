/*
 * uRISCV - A general purpose computer system simulator
 *
 * Copyright (C) 2010 Tomislav Jonjic
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

#ifndef QRISCV_PROCESSOR_WINDOW_H
#define QRISCV_PROCESSOR_WINDOW_H

#include <QMainWindow>

#include "uriscv/types.h"

class QToolBar;
class Processor;
class DebugSession;
class QLabel;
class CodeView;
class QLayout;
class QDockWidget;
class RegisterSetWidget;

class ProcessorWindow : public QMainWindow {
  Q_OBJECT

public:
  ProcessorWindow(Word cpuId, QWidget *parent = 0);

protected:
  virtual void closeEvent(QCloseEvent *event);

private:
  void createMenu();
  void createToolBar();
  QLayout *createInstrPanel();
  void createDockableWidgets();

  DebugSession *const dbgSession;
  const Word cpuId;
  Processor *cpu;

  QToolBar *toolBar;
  QLabel *statusLabel;
  CodeView *codeView;

  QLabel *prevPCLabel;
  QLabel *prevInstructionLabel;
  QLabel *pcLabel;
  QLabel *instructionLabel;

  QLabel *bdIndicator;
  QLabel *ldIndicator;

  RegisterSetWidget *regView;
  QDockWidget *tlbWidget;

private Q_SLOTS:
  void onMachineReset();
  void updateStatusInfo();
  void updateTLBViewTitle(bool topLevel);
};

#endif // QRISCV_PROCESSOR_WINDOW_H
