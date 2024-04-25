/*
 * uRISCV - A general purpose computer system simulator
 *
 * Copyright (C) 2010, 2011 Tomislav Jonjic
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

#include "qriscv/register_item_delegate.h"

#include <QIntValidator>
#include <QLineEdit>
#include <QValidator>

#include "qriscv/application.h"

class RIValidator : public QValidator {
public:
  RIValidator(QObject *parent = 0) : QValidator(parent) {}

  virtual State validate(QString &input, int &pos) const;
};

QValidator::State RIValidator::validate(QString &input, int &pos) const {
  UNUSED_ARG(pos);
  input.replace(' ', '0');
  return Acceptable;
}

QString RegisterItemDelegate::displayText(const QVariant &variant,
                                          const QLocale &locale) const {
  if (variant.canConvert<Word>())
    return Text(variant.value<Word>());
  else
    return QStyledItemDelegate::displayText(variant, locale);
}

void RegisterItemDelegate::setEditorData(QWidget *editor,
                                         const QModelIndex &index) const {
  QLineEdit *lineEdit = static_cast<QLineEdit *>(editor);
  lineEdit->setText(Text(index.data().value<Word>()));
}

void RegisterItemDelegate::updateEditorGeometry(
    QWidget *editor, const QStyleOptionViewItem &option,
    const QModelIndex &index) const {
  UNUSED_ARG(index);
  editor->setGeometry(option.rect);
}

QWidget *RIDelegateHex::createEditor(QWidget *parent,
                                     const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
  UNUSED_ARG(option);
  UNUSED_ARG(index);
  QLineEdit *editor = new QLineEdit(parent);
  editor->setInputMask("\\0\\xHHHHHHHH");
  editor->setValidator(new RIValidator(editor));
  editor->setFont(Appl()->getMonospaceFont());
  return editor;
}

void RIDelegateHex::setModelData(QWidget *editor, QAbstractItemModel *model,
                                 const QModelIndex &index) const {
  QLineEdit *lineEdit = static_cast<QLineEdit *>(editor);
  Word data = lineEdit->text().remove(0, 2).toUInt(0, 16);
  model->setData(index, data, Qt::EditRole);
}

QWidget *
RIDelegateSignedDecimal::createEditor(QWidget *parent,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const {
  UNUSED_ARG(option);
  UNUSED_ARG(index);
  QLineEdit *editor = new QLineEdit(parent);
  editor->setValidator(new QIntValidator(editor));
  editor->setFont(Appl()->getMonospaceFont());
  return editor;
}

void RIDelegateSignedDecimal::setModelData(QWidget *editor,
                                           QAbstractItemModel *model,
                                           const QModelIndex &index) const {
  QLineEdit *lineEdit = static_cast<QLineEdit *>(editor);
  Word data = (Word)lineEdit->text().toInt(0, 10);
  model->setData(index, data, Qt::EditRole);
}

QWidget *
RIDelegateUnsignedDecimal::createEditor(QWidget *parent,
                                        const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const {
  UNUSED_ARG(option);
  UNUSED_ARG(index);
  QLineEdit *editor = new QLineEdit(parent);
  QIntValidator *validator = new QIntValidator(editor);
  validator->setBottom(0);
  editor->setValidator(validator);
  editor->setFont(Appl()->getMonospaceFont());
  return editor;
}

void RIDelegateUnsignedDecimal::setModelData(QWidget *editor,
                                             QAbstractItemModel *model,
                                             const QModelIndex &index) const {
  QLineEdit *lineEdit = static_cast<QLineEdit *>(editor);
  model->setData(index, lineEdit->text().toUInt(0, 10), Qt::EditRole);
}

QWidget *RIDelegateBinary::createEditor(QWidget *parent,
                                        const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const {
  UNUSED_ARG(option);
  UNUSED_ARG(index);
  QLineEdit *editor = new QLineEdit(parent);
  editor->setInputMask("BBBBBBBB|BBBBBBBB|BBBBBBBB|BBBBBBBB");
  editor->setValidator(new RIValidator(editor));
  editor->setFont(Appl()->getMonospaceFont());
  return editor;
}

void RIDelegateBinary::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const {
  QLineEdit *lineEdit = static_cast<QLineEdit *>(editor);
  Word data = lineEdit->text().remove("|").toUInt(0, 2);
  model->setData(index, data, Qt::EditRole);
}

QString RIDelegateBinary::Text(Word value) const {
  return (QString("%1|%2|%3|%4")
              .arg((value >> 24) & 0xFFU, 8, 2, QLatin1Char('0'))
              .arg((value >> 16) & 0xFFU, 8, 2, QLatin1Char('0'))
              .arg((value >> 8) & 0xFFU, 8, 2, QLatin1Char('0'))
              .arg((value >> 0) & 0xFFU, 8, 2, QLatin1Char('0')));
}
