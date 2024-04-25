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

#ifndef QRISCV_TREE_VIEW_H
#define QRISCV_TREE_VIEW_H

#include <list>

#include <QTreeView>

class QAbstractItemModel;

class TreeView : public QTreeView {
  Q_OBJECT

public:
  TreeView(const QString &name, const std::list<int> &resizedToContents,
           bool persistItemState = false, QWidget *parent = 0);

  void setModel(QAbstractItemModel *model);

private Q_SLOTS:
  void sectionResized(int logicalIndex, int oldSize, int newSize);
  void saveItemState();

private:
  const std::list<int> resizedToContents;
  const bool persistItemState;
  QString itemStateKey;
};

#endif // QRISCV_TREE_VIEW_H
