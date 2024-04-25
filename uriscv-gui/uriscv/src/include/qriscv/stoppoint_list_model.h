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

#ifndef QRISCV_STOPPINT_LIST_MODEL_H
#define QRISCV_STOPPINT_LIST_MODEL_H

#include <sigc++/sigc++.h>
#include <vector>

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QString>

#include "base/basic_types.h"
#include "base/lang.h"
#include "base/trackable_mixin.h"
#include "uriscv/stoppoint.h"

class SymbolTable;

class BaseStoppointListModel : public QAbstractTableModel {
  Q_OBJECT

public:
  BaseStoppointListModel(StoppointSet *set, QObject *parent = 0);

  int rowCount(const QModelIndex &parent = QModelIndex()) const;

  // We have to proxy these operations because of annoying
  // QAbstractItemModel API obligations.
  bool Add(const AddressRange &range, AccessMode mode);
  void Remove(int index);
  void Remove(Stoppoint *sp);

protected:
  QString getAddressRange(int i) const;

  virtual void StoppointAdded() {}
  virtual void StoppointRemoved(int index) { UNUSED_ARG(index); }

  StoppointSet *stoppoints;

private:
  QString formatAddressRange(const AddressRange &range);

  std::vector<QString> formattedRangeCache;
  SymbolTable *const symbolTable;
};

class StoppointListModel : public BaseStoppointListModel,
                           public TrackableMixin {
  Q_OBJECT

public:
  enum {
    COLUMN_STOPPOINT_ID = 0,
    COLUMN_ACCESS_TYPE,
    COLUMN_ASID,
    COLUMN_ADDRESS_RANGE,
    COLUMN_VICTIMS,
    N_COLUMNS
  };

  StoppointListModel(StoppointSet *stoppoints, const char *collectionName,
                     char idPrefix, QObject *parent = 0);

  int columnCount(const QModelIndex &parent) const;

  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  QVariant data(const QModelIndex &index, int role) const;

  Qt::ItemFlags flags(const QModelIndex &index) const;

  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole);

protected:
  virtual void StoppointAdded();
  virtual void StoppointRemoved(int index);

private:
  void onHit(size_t index, const Stoppoint *stoppoint, Word addr,
             const Processor *cpu);
  void onEnabledChanged(size_t spIndex);

  static const char *const headers[N_COLUMNS];

  const char *const collectionName;
  char idPrefix;

  std::vector<uint32_t> victims;

private Q_SLOTS:
  void onMachineRan();
};

#endif // QRISCV_STOPPINT_LIST_MODEL_H
