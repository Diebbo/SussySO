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

#include <QAction>
#include <QCheckBox>
#include <QGridLayout>

#include "qriscv/stop_mask_view.h"

StopMaskView::StopMaskView(const std::map<StopCause, QAction *> &actions,
                           QWidget *parent)
    : QGroupBox("Stop Mask", parent) {
  QGridLayout *layout = new QGridLayout;

  std::map<StopCause, QAction *>::const_iterator it;
  int col = 0;
  for (it = actions.begin(); it != actions.end(); ++it) {
    QAction *action = it->second;
    QCheckBox *cb = new QCheckBox(action->text());
    cb->setChecked(action->isChecked());
    connect(action, SIGNAL(triggered(bool)), cb, SLOT(setChecked(bool)));
    connect(cb, SIGNAL(toggled(bool)), action, SLOT(setChecked(bool)));
    layout->addWidget(cb, 0, col++, Qt::AlignLeft);
  }
  --col;
  layout->setColumnStretch(col, 1);
  layout->setHorizontalSpacing(11);

  setLayout(layout);
}
