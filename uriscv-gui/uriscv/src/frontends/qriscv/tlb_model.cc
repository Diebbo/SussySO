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

#include "qriscv/tlb_model.h"

#include <cassert>

#include "qriscv/application.h"
#include "uriscv/processor.h"
#include "uriscv/processor_defs.h"
#include "uriscv/utility.h"

const char *const TLBModel::detailsTemplate = "     \
<center><b>TLB Entry %EntryNo%</b></center>         \
<table cellspacing='11'>                            \
  <tr>                                              \
    <th colspan=\"2\">EntryHi</th>                  \
    <th colspan=\"2\">EntryLo</th>                  \
  </tr>                                             \
  <tr>                                              \
    <th align='right'>VPN:</th>                     \
    <td align='left'>0x%EntryHi.VPN%</td>           \
    <th align='right'>PFN:</th>                     \
    <td align='left'>0x%EntryLo.PFN%</td>           \
  </tr>                                             \
  <tr>                                              \
    <th align='right'>ASID:</th>                    \
    <td align='left'>0x%EntryHi.ASID%</td>          \
    <th align='right'>D:</th>                       \
    <td align='left'>%EntryLo.D%</td>               \
  </tr>                                             \
  <tr>                                              \
    <th></th>                                       \
    <td></td>                                       \
    <th align='right'>V:</th>                       \
    <td align='left'>%EntryLo.V%</td>               \
  </tr>                                             \
  <tr>                                              \
    <th></th>                                       \
    <td></td>                                       \
    <th align='right'>G:</th>                       \
    <td align='left'>%EntryLo.G%</td>               \
  </tr>                                             \
</table>                                            \
";

TLBModel::TLBModel(Word cpuId, QObject *parent)
    : QAbstractTableModel(parent), cpuId(cpuId) {
  connect(debugSession, SIGNAL(MachineReset()), this, SLOT(onMachineReset()));
  onMachineReset();
}

Qt::ItemFlags TLBModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::NoItemFlags;

  return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

int TLBModel::rowCount(const QModelIndex &parent) const {
  if (!parent.isValid())
    return Appl()->getConfig()->getTLBSize();
  else
    return 0;
}

int TLBModel::columnCount(const QModelIndex &parent) const {
  if (!parent.isValid())
    return N_COLUMNS;
  else
    return 0;
}

QVariant TLBModel::headerData(int section, Qt::Orientation orientation,
                              int role) const {
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal) {
    switch (section) {
    case COLUMN_PTE_HI:
      return "EntryHi";
    case COLUMN_PTE_LO:
      return "EntryLo";
    default:
      return QVariant();
    }
  }

  if (orientation == Qt::Vertical)
    return section;

  return QVariant();
}

QVariant TLBModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    switch (index.column()) {
    case COLUMN_PTE_HI:
      return cpu->getTLBHi(index.row());
    case COLUMN_PTE_LO:
      return cpu->getTLBLo(index.row());
    default:
      return QVariant();
    }
  }

  if (role == Qt::ToolTipRole)
    return tlbEntryDetails(index.row());

  if (role == Qt::FontRole)
    return Appl()->getMonospaceFont();

  return QVariant();
}

bool TLBModel::setData(const QModelIndex &index, const QVariant &value,
                       int role) {
  if (!(index.isValid() && role == Qt::EditRole && value.canConvert<Word>()))
    return false;

  switch (index.column()) {
  case COLUMN_PTE_HI:
    cpu->setTLBHi(index.row(), value.value<Word>());
    break;
  case COLUMN_PTE_LO:
    cpu->setTLBLo(index.row(), value.value<Word>());
    break;
  default:
    // Assert not reached
    assert(0);
  }

  return true;
}

void TLBModel::onMachineReset() {
  beginResetModel();
  cpu = debugSession->getMachine()->getProcessor(cpuId);
  cpu->SignalTLBChanged.connect(sigc::mem_fun(this, &TLBModel::onTLBChanged));
  endResetModel();
}

void TLBModel::onTLBChanged(unsigned int tlbIndex) {
  Q_EMIT dataChanged(index(tlbIndex, COLUMN_PTE_HI),
                     index(tlbIndex, COLUMN_PTE_LO));
}

QString TLBModel::tlbEntryDetails(unsigned int index) const {
  Word hi = cpu->getTLBHi(index);
  Word lo = cpu->getTLBLo(index);

  QString buf(detailsTemplate);
  buf.replace("%EntryNo%", QString::number(index));

  buf.replace("%EntryHi.VPN%",
              QString("%1").arg(VPN(hi) >> 12, 5, 16, QLatin1Char('0')));
  buf.replace("%EntryHi.ASID%", QString::number(ASID(hi) >> 6, 16));

  buf.replace("%EntryLo.PFN%",
              QString("%1").arg(VPN(lo) >> 12, 5, 16, QLatin1Char('0')));
  buf.replace("%EntryLo.D%", QString::number(BitVal(lo, DBITPOS)));
  buf.replace("%EntryLo.V%", QString::number(BitVal(lo, VBITPOS)));
  buf.replace("%EntryLo.G%", QString::number(BitVal(lo, GBITPOS)));

  return buf;
}
