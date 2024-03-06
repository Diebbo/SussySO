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

#include "qriscv/symbol_table_model.h"

#include <cassert>

#include "qriscv/application.h"
#include "qriscv/debug_session.h"
#include "uriscv/symbol_table.h"

SymbolTableModel::SymbolTableModel(QObject *parent)
    : QAbstractTableModel(parent),
      table(Appl()->getDebugSession()->getSymbolTable()) {}

int SymbolTableModel::rowCount(const QModelIndex &parent) const {
  if (!parent.isValid())
    return table->Size();
  else
    return 0;
}

int SymbolTableModel::columnCount(const QModelIndex &parent) const {
  if (!parent.isValid())
    return N_COLUMNS;
  else
    return 0;
}

QVariant SymbolTableModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch (section) {
    case COLUMN_SYMBOL:
      return "Symbol";
    case COLUMN_START_ADDRESS:
      return "Start";
    case COLUMN_END_ADDRESS:
      return "End";
    }
  }
  return QVariant();
}

QVariant SymbolTableModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (role == Qt::DisplayRole) {
    const Symbol *symbol = table->Get(index.row());
    switch (index.column()) {
    case COLUMN_SYMBOL:
      return symbol->getName();
    case COLUMN_START_ADDRESS:
      return QString("0x%1").arg((quint32)symbol->getStart(), 8, 16,
                                 QChar('0'));
    case COLUMN_END_ADDRESS:
      return QString("0x%1").arg((quint32)symbol->getEnd(), 8, 16, QChar('0'));
    default:
      return QVariant();
    }
  } else if (role == Qt::FontRole) {
    return Appl()->getMonospaceFont();
  }

  return QVariant();
}

SortFilterSymbolTableModel::SortFilterSymbolTableModel(Symbol::Type type,
                                                       QObject *parent)
    : QSortFilterProxyModel(parent),
      table(Appl()->getDebugSession()->getSymbolTable()), tableType(type) {
  setSortCaseSensitivity(Qt::CaseInsensitive);
}

QVariant SortFilterSymbolTableModel::headerData(int section,
                                                Qt::Orientation orientation,
                                                int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole &&
      section == SymbolTableModel::COLUMN_SYMBOL) {
    if (tableType == Symbol::TYPE_OBJECT)
      return "Object";
    else
      return "Function";
  }

  return QSortFilterProxyModel::headerData(section, orientation, role);
}

bool SortFilterSymbolTableModel::filterAcceptsRow(
    int sourceRow, const QModelIndex &sourceParent) const {
  return table->Get(sourceRow)->getType() == tableType;
}
