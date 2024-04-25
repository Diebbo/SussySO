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

#include <iostream>

#include <cassert>

#include "qriscv/application.h"
#include "qriscv/cpu_status_map.h"
#include "qriscv/processor_list_model.h"
#include "uriscv/machine.h"
#include "uriscv/processor.h"

const char *ProcessorListModel::headers[ProcessorListModel::N_COLUMNS] = {
    "Processor", "Status", "Location"};

ProcessorListModel::ProcessorListModel(QObject *parent)
    : QAbstractTableModel(parent), config(Appl()->getConfig()),
      dbgSession(Appl()->getDebugSession()),
      cpuStatusMap(dbgSession->getCpuStatusMap()) {
  connect(cpuStatusMap, SIGNAL(Changed()), this, SLOT(notifyStatusChanged()));
}

int ProcessorListModel::rowCount(const QModelIndex &parent) const {
  if (!parent.isValid())
    return config->getNumProcessors();
  else
    return 0;
}

int ProcessorListModel::columnCount(const QModelIndex &parent) const {
  if (!parent.isValid())
    return N_COLUMNS;
  else
    return 0;
}

QVariant ProcessorListModel::headerData(int section,
                                        Qt::Orientation orientation,
                                        int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return headers[section];
  else
    return QVariant();
}

QVariant ProcessorListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  switch (index.column()) {
  case COLUMN_CPU_ID:
    if (role == Qt::DisplayRole)
      return index.row();
    break;

  case COLUMN_CPU_STATUS:
    if (role == Qt::DisplayRole)
      return cpuStatusMap->getStatus(index.row());
    break;

  case COLUMN_CPU_ADDRESS:
    if (role == Qt::DisplayRole)
      return cpuStatusMap->getLocation(index.row());
    if (role == Qt::FontRole)
      return Appl()->getMonospaceFont();

  default:
    return QVariant();
  }

  return QVariant();
}

void ProcessorListModel::notifyStatusChanged() {
  for (unsigned int i = 0; i < config->getNumProcessors(); i++) {
    QModelIndex idx0 = index(i, COLUMN_CPU_STATUS);
    QModelIndex idx1 = index(i, COLUMN_CPU_ADDRESS);
    Q_EMIT dataChanged(idx0, idx1);
  }
}
