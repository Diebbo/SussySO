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

#ifndef QRISCV_TLB_MODEL_H
#define QRISCV_TLB_MODEL_H

#include <sigc++/sigc++.h>

#include <QAbstractTableModel>

#include "uriscv/types.h"

class Processor;

class TLBModel : public QAbstractTableModel, public sigc::trackable {
  Q_OBJECT

public:
  enum Column { COLUMN_PTE_HI, COLUMN_PTE_LO, N_COLUMNS };

  TLBModel(Word cpuId, QObject *parent = 0);

  Qt::ItemFlags flags(const QModelIndex &index) const;

  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole);

private Q_SLOTS:
  void onMachineReset();

private:
  static const char *const detailsTemplate;

  void onTLBChanged(unsigned int index);
  QString tlbEntryDetails(unsigned int index) const;

  const Word cpuId;
  Processor *cpu;
};

#endif // QRISCV_TLB_MODEL_H
