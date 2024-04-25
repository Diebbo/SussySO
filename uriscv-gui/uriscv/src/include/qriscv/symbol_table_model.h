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

#ifndef QRISCV_SYMBOL_TABLE_MODEL_H
#define QRISCV_SYMBOL_TABLE_MODEL_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include "uriscv/symbol_table.h"

class SymbolTableModel : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Column {
    COLUMN_SYMBOL,
    COLUMN_START_ADDRESS,
    COLUMN_END_ADDRESS,
    N_COLUMNS
  };

  SymbolTableModel(QObject *parent = 0);

  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;

  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  QVariant data(const QModelIndex &index, int role) const;

private:
  const SymbolTable *const table;
};

class SortFilterSymbolTableModel : public QSortFilterProxyModel {
  Q_OBJECT

public:
  SortFilterSymbolTableModel(Symbol::Type tableType, QObject *parent = 0);

  virtual QVariant headerData(int section, Qt::Orientation orientation,
                              int role = Qt::DisplayRole) const;

protected:
  virtual bool filterAcceptsRow(int sourceRow,
                                const QModelIndex &sourceParent) const;

private:
  const SymbolTable *const table;
  const Symbol::Type tableType;
};

#endif // QRISCV_SYMBOL_TABLE_MODEL_H
