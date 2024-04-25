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

#ifndef QRISCV_HEX_VIEW_PRIV_H
#define QRISCV_HEX_VIEW_PRIV_H

#include <QWidget>

class HexView;

class HexViewMargin : public QWidget {
  Q_OBJECT

public:
  static const int kLeftPadding = 3;
  static const int kRightPadding = 5;

  HexViewMargin(HexView *hexView);

  virtual QSize sizeHint() const;

protected:
  void paintEvent(QPaintEvent *event);
  void wheelEvent(QWheelEvent *event);

private:
  HexView *const hexView;
};

#endif // QRISCV_HEX_VIEW_PRIV_H
