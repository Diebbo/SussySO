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

#include "qriscv/code_view.h"

#include <boost/bind/bind.hpp>
#include <iterator>
#include <list>

#include <QByteArray>
#include <QFont>
#include <QPainter>
#include <QPixmap>
#include <QTextBlock>
#include <QToolTip>

#include "base/lang.h"
#include "qriscv/application.h"
#include "qriscv/code_view_priv.h"
#include "qriscv/debug_session.h"
#include "qriscv/stoppoint_list_model.h"
#include "qriscv/ui_utils.h"
#include "uriscv/disassemble.h"
#include "uriscv/machine.h"
#include "uriscv/machine_config.h"
#include "uriscv/processor.h"
#include "uriscv/processor_defs.h"
#include "uriscv/stoppoint.h"
#include "uriscv/symbol_table.h"

using namespace boost::placeholders;

CodeView::CodeView(Word cpuId)
    : QPlainTextEdit(), codeMargin(new CodeViewMargin(this)),
      dbgSession(Appl()->getDebugSession()), cpuId(cpuId),
      pcMarkerPixmap(":/icons/pc-16.svg"),
      enabledBpMarkerPixmap(":/icons/breakpoint_enabled-16.svg"),
      disabledBpMarkerPixmap(":/icons/breakpoint_disabled-16.svg") {
  setLineWrapMode(NoWrap);
  setReadOnly(true);

  QFont font = Appl()->getMonospaceFont();
  setFont(font);
  codeMargin->setFont(font);
  codeMargin->setCursor(Qt::PointingHandCursor);

  setTabStopDistance(fontMetrics().horizontalAdvance("x") * TAB_STOP_CHARS);

  // Compute viewport margins
  setViewportMargins(codeMargin->sizeHint().width(), 0, 0, 0);

  // The updateRequest() signal is emitted when QPlainTextEdit has
  // to update its viewport, e.g. after a vertical or horizontal
  // scroll. This signal was in fact devised especially for
  // QPlainTextEdit subclasses that want to add surrounding content
  // to the widget.
  connect(this, SIGNAL(updateRequest(QRect, int)), this,
          SLOT(updateMargin(QRect, int)));

  // Connect to debugger events of interest
  connect(dbgSession, SIGNAL(MachineStopped()), this, SLOT(onMachineStopped()));
  connect(dbgSession, SIGNAL(MachineRan()), this, SLOT(update()));
  connect(dbgSession, SIGNAL(MachineReset()), this, SLOT(reset()));

  disasmMap[B_TYPE] = boost::bind(&CodeView::disasmBranch, this, _1, _2);
  disasmMap[OP_JAL] = boost::bind(&CodeView::disasmJump, this, _1, _2);
  disasmMap[OP_JALR] = boost::bind(&CodeView::disasmJump, this, _1, _2);

  reset();
}

// We need to handle resize events since we contain a child widget (a
// margin); on any resize, we must resize the margin accordingly.

void CodeView::resizeEvent(QResizeEvent *event) {
  QPlainTextEdit::resizeEvent(event);

  // The contentsRect() function gives us the complete area
  // (geometry) assigned to the code view, _including_ the margins,
  // so we can finally "materialize" the margin: it will lay at the
  // top left corner of the abovementioned rect, has the codeviews'
  // height and its desired width.
  QRect rect = contentsRect();
  codeMargin->setGeometry(QRect(rect.left(), rect.top(),
                                codeMargin->sizeHint().width(), rect.height()));
}

void CodeView::loadCode() {
  const MachineConfig *config = Appl()->getConfig();
  Machine *machine = dbgSession->getMachine();

  codeLoaded = false;
  clear();

  if (cpu->isHalted())
    return;

  Word pc = cpu->getPC();
  if (pc >= RAM_BASE) {
    if (pc < config->getTLBFloorAddress() &&
        config->getSymbolTableASID() == MAXASID) {
      const Symbol *symbol = symbolTable->Probe(MAXASID, pc, false);
      if (symbol != NULL) {
        startPC = symbol->getStart();
        endPC = symbol->getEnd();
        codeLoaded = true;
      }
    }
  } else if (pc >= KSEG0_BOOT_BASE) {
    Word bootSize;
    machine->ReadMemory(BUS_REG_BOOT_SIZE, &bootSize);
    if (pc <= KSEG0_BOOT_BASE + bootSize - WS) {
      startPC = KSEG0_BOOT_BASE;
      endPC = startPC + bootSize - WS;
      codeLoaded = true;
    }
  } else {
    Word biosSize;
    machine->ReadMemory(BUS_REG_BIOS_SIZE, &biosSize);
    if (pc <= KSEG0_BIOS_BASE + biosSize - WS) {
      startPC = KSEG0_BIOS_BASE;
      endPC = startPC + biosSize - WS;
      codeLoaded = true;
    }
  }

  if (codeLoaded) {
    for (Word addr = startPC; addr <= endPC; addr += WS) {
      Word instr;
      machine->ReadMemory(addr, &instr);
      appendPlainText(disassemble(instr, addr));
    }
    ensureCurrentInstructionVisible();
  }
}

void CodeView::onBreakpointInserted() { update(); }

void CodeView::onBreakpointChanged(size_t) { update(); }

QString CodeView::disassemble(Word instr, Word pc) const {
  DisasmMap::const_iterator it = disasmMap.find(OPCODE(instr));
  if (it != disasmMap.end())
    return it->second(instr, pc);
  else
    return StrInstr(instr);
}

QString CodeView::disasmBranch(Word instr, Word pc) const {
  Word target = pc + SIGN_EXTENSION(B_IMM(instr), I_IMM_SIZE);

#if 0
	// Resolve symbol, if possible
	SWord offset;
	const char* symbol = GetSymbolicAddress(symbolTable, MachineConfig::MAX_ASID, target, true, &offset);

	return (QString("%1\t%2, %3, %4 %5")
	        .arg(getBInstrName(instr))
	        .arg(RegName(RS1(instr)))
	        .arg(RegName(RS2(instr)))
	        .arg(target, 8, 16, QChar('0'))
	        .arg(symbol ? QString(" <%1+0x%2>").arg(symbol).arg(offset, 0, 16) : QString()));
#else
  return (QString("%1\t%2, %3, 0x%4")
              .arg(getBInstrName(instr))
              .arg(RegName(RS1(instr)))
              .arg(RegName(RS2(instr)))
              .arg(target, 8, 16, QChar('0')));
#endif
}

QString CodeView::disasmJump(Word instr, Word pc) const {
  if (instr == INSTR_RET)
    return QString("ret");

  Word immediate = OPCODE(instr) == OP_JAL
                       ? SIGN_EXTENSION(J_IMM(instr), J_IMM_SIZE)
                       : SIGN_EXTENSION(I_IMM(instr), I_IMM_SIZE);
  const char *instrName = OPCODE(instr) == OP_JAL ? "jal" : "jalr";
  Word target = pc + immediate;

  SWord offset;
  const char *symbol = GetSymbolicAddress(symbolTable, MachineConfig::MAX_ASID,
                                          target, true, &offset);
  return (QString("%1\t0x%2%3")
              .arg(instrName)
              .arg(target, 8, 16, QChar('0'))
              .arg(symbol ? QString(" <%1+0x%2>").arg(symbol).arg(offset, 0, 16)
                          : QString()));
}

void CodeView::onMachineStopped() {
  const MachineConfig *config = Appl()->getConfig();
  Word pc = cpu->getPC();

  if (!codeLoaded) {
    loadCode();
  } else {
    if (pc >= config->getTLBFloorAddress()) {
      clear();
      codeLoaded = false;
    } else if (startPC <= pc && pc <= endPC) {
      codeMargin->update();
      ensureCurrentInstructionVisible();
    } else {
      loadCode();
    }
  }
}

void CodeView::updateMargin(const QRect &rect, int dy) {
  if (dy != 0)
    codeMargin->scroll(0, dy);
  else
    codeMargin->update(0, rect.y(), codeMargin->width(), rect.height());
}

void CodeView::reset() {
  cpu = dbgSession->getMachine()->getProcessor(cpuId);

  breakpoints = dbgSession->getBreakpoints();
  breakpoints->SignalStoppointInserted.connect(
      sigc::mem_fun(this, &CodeView::onBreakpointInserted));
  breakpoints->SignalStoppointRemoved.connect(
      sigc::mem_fun(this, &CodeView::onBreakpointChanged));
  breakpoints->SignalEnabledChanged.connect(
      sigc::mem_fun(this, &CodeView::onBreakpointChanged));

  symbolTable = dbgSession->getSymbolTable();
  bplModel = dbgSession->getBreakpointListModel();

  loadCode();
}

void CodeView::paintMargin(QPaintEvent *event) {
  QPainter painter(codeMargin);

  painter.fillRect(event->rect(), palette().window().color());

  // Nothing left to do unless we are displaying a code block.
  if (!codeLoaded)
    return;

  painter.setPen(palette().windowText().color());

  QTextBlock block = firstVisibleBlock();
  if (!block.isValid())
    return;

  std::list<Stoppoint *> localBreakpoints;
  breakpoints->GetStoppointsInRange(MachineConfig::MAX_ASID,
                                    startPC + block.blockNumber() * WS, endPC,
                                    std::back_inserter(localBreakpoints));

  for (Stoppoint *breakpoint : localBreakpoints) {
    unsigned int bpOffset = (breakpoint->getRange().getStart() - startPC) >> 2;
    QTextBlock b = document()->findBlockByNumber(bpOffset);
    int y0 = (int)blockBoundingGeometry(b).translated(contentOffset()).y();
    if (y0 > event->rect().bottom())
      break;
    int y1 = y0 + (int)blockBoundingRect(b).height();
    if (event->rect().top() <= y1)
      painter.drawPixmap(codeMargin->width() - CodeViewMargin::kMarkerSize, y0,
                         breakpoint->IsEnabled() ? enabledBpMarkerPixmap
                                                 : disabledBpMarkerPixmap);
  }

  Word addr = startPC + block.blockNumber() * WS;

  int y0 = (int)blockBoundingGeometry(block).translated(contentOffset()).y();
  int y1 = y0 + (int)blockBoundingRect(block).height();

  Word pc = cpu->getPC();

  while (y0 <= event->rect().bottom()) {
    if (event->rect().top() <= y1) {
      if (addr == pc && dbgSession->isStopped())
        painter.drawPixmap(codeMargin->width() - CodeViewMargin::kMarkerSize,
                           y0, pcMarkerPixmap);
      painter.drawText(0, y0, codeMargin->width(),
                       codeMargin->fontMetrics().height(), Qt::AlignLeft,
                       FormatAddress(addr));
    }
    block = block.next();
    if (!block.isValid())
      break;
    y0 = y1;
    y1 += blockBoundingRect(block).height();
    addr += WS;
  }
}

void CodeView::ensureCurrentInstructionVisible() {
  QTextCursor cursor = textCursor();
  cursor.setPosition(0);
  unsigned int offset = (cpu->getPC() - startPC) >> 2;
  cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, offset);
  setTextCursor(cursor);
}

CodeViewMargin::CodeViewMargin(CodeView *view)
    : QWidget(view), codeView(view) {}

QSize CodeViewMargin::sizeHint() const {
  QString sampleAddr = FormatAddress(0xdeadbeef);
  return QSize(fontMetrics().horizontalAdvance("o") * sampleAddr.size() +
                   kMarkerSize,
               0);
}

void CodeViewMargin::paintEvent(QPaintEvent *event) {
  codeView->paintMargin(event);
}

void CodeViewMargin::mousePressEvent(QMouseEvent *event) {
  if (event->button() != Qt::LeftButton || !codeView->codeLoaded)
    return;

  int index = indexAt(event->pos());
  if (index == -1)
    return;

  Word addr = codeView->startPC + index * WS;

  Stoppoint *p = codeView->breakpoints->Find(MachineConfig::MAX_ASID, addr);
  if (p == NULL)
    codeView->bplModel->Add(AddressRange(MachineConfig::MAX_ASID, addr, addr),
                            AM_EXEC);
  else
    codeView->bplModel->Remove(p);
}

bool CodeViewMargin::event(QEvent *event) {
  if (event->type() == QEvent::ToolTip) {
    QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
    int index = indexAt(helpEvent->pos());
    if (index != -1) {
      Word addr = codeView->startPC + index * WS;
      Stoppoint *p = codeView->breakpoints->Find(MachineConfig::MAX_ASID, addr);
      if (p != NULL)
        QToolTip::showText(helpEvent->globalPos(),
                           QString("Breakpoint B%1").arg(p->getId()));
    } else {
      QToolTip::hideText();
      event->ignore();
    }
    return true;
  }
  return QWidget::event(event);
}

void CodeViewMargin::wheelEvent(QWheelEvent *event) {
  codeView->wheelEvent(event);
}

int CodeViewMargin::indexAt(const QPoint &pos) const {
  QTextCursor cursor = codeView->cursorForPosition(pos);
  QTextBlock block = cursor.block();
  return block.isValid() ? block.blockNumber() : -1;
}
