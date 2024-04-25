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

#ifndef QRISCV_ADD_SUSPECT_DIALOG_H
#define QRISCV_ADD_SUSPECT_DIALOG_H

#include <QDialog>

#include "uriscv/types.h"

class AddressLineEdit;
class AsidLineEdit;
class SymbolTable;
class SortFilterSymbolTableModel;
class QItemSelection;
class QPushButton;

class AddSuspectDialog : public QDialog {
  Q_OBJECT

public:
  AddSuspectDialog(QWidget *parent = 0);

  Word getStartAddress() const;
  Word getEndAddress() const;
  Word getASID() const;

private:
  static const int kInitialWidth = 380;
  static const int kInitialHeight = 340;

  AsidLineEdit *asidEditor;
  AddressLineEdit *startAddressEdit;
  AddressLineEdit *endAddressEdit;

  const SymbolTable *const stab;
  SortFilterSymbolTableModel *proxyModel;

  QPushButton *okButton;

private Q_SLOTS:
  void validate();
  void onSelectionChanged(const QItemSelection &selected);
};

#endif // QRISCV_ADD_SUSPECT_DIALOG_H
