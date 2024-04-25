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

#include "qriscv/address_line_edit.h"

#include <cassert>

#include <QValidator>

#include "base/lang.h"
// #include "uriscv/umps/const.h"
#include "qriscv/application.h"

class AddressValidator : public QValidator {
public:
  AddressValidator(QObject *parent = 0) : QValidator(parent) {}

  virtual State validate(QString &input, int &pos) const;
};

QValidator::State AddressValidator::validate(QString &input, int &pos) const {
  UNUSED_ARG(pos);
  input.replace(' ', '0');
  // Beware of the non-const reference parameter; make a copy since
  // we need to modify it for convenience.
  QString temp = input;
  bool ok;
  unsigned int value = temp.remove(0, 2).remove(4, 1).toUInt(&ok, 16);
  // What's left _has_ to be valid hex _already_!
  assert(ok);
  return (value & 3U) ? Invalid : Acceptable;
}

AddressLineEdit::AddressLineEdit(QWidget *parent) : QLineEdit(parent) {
  setInputMask("\\0\\xHHHH.HHHH");
  setValidator(new AddressValidator(this));
  setFont(Appl()->getMonospaceFont());
  setText("0000.0000");
}

Word AddressLineEdit::getAddress() const {
  bool ok;
  Word result = text().remove(0, 2).remove(4, 1).toUInt(&ok, 16);
  // Better be ok! Otherwise our validation is broken.
  assert(ok);
  return result;
}

void AddressLineEdit::setAddress(Word addr) {
  if (!(addr & 3U)) {
    setText(QString("0x%1.%2")
                .arg((quint32)(addr >> 16), 4, 16, QChar('0'))
                .arg((quint32)(addr & 0x0000ffffU), 4, 16, QChar('0')));
  }
}

class AsidValidator : public QValidator {
public:
  AsidValidator(QObject *parent = 0) : QValidator(parent) {}

  virtual State validate(QString &input, int &pos) const;
};

QValidator::State AsidValidator::validate(QString &input, int &pos) const {
  if (input.endsWith(' ')) {
    input.remove(3, 1);
    input.insert(2, '0');
  }
  input.replace(' ', '0');
  QString temp = input;
  bool ok;
  unsigned int value = temp.remove(0, 2).toUInt(&ok, 16);
  // What's left _has_ to be valid hex _already_!
  assert(ok);
  return (value <= MachineConfig::MAX_ASID) ? Acceptable : Invalid;
}

AsidLineEdit::AsidLineEdit(QWidget *parent) : QLineEdit(parent) {
  setInputMask("\\0\\xHH");
  setValidator(new AsidValidator(this));
  setFont(Appl()->getMonospaceFont());
  setText("00");
}

Word AsidLineEdit::getAsid() const {
  bool ok;
  Word result = text().remove(0, 2).toUInt(&ok, 16);
  assert(ok);
  return result;
}

void AsidLineEdit::setAsid(Word asid) {
  if (asid <= MachineConfig::MAX_ASID)
    setText(QString::number(asid, 16));
}
