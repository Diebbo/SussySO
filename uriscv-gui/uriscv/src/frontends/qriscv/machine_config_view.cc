/*
 * uRISCV - A general purpose computer system simulator
 *
 * Copyright (C) 2010 Tomislav Jonjic
 * Copyright (C) 2020 Mattia Biondi
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

#include "qriscv/machine_config_view.h"
#include "qriscv/ui_utils.h"

#include <QGridLayout>
#include <QLabel>

#include "qriscv/application.h"
#include "uriscv/const.h"
#include "uriscv/machine_config.h"

MachineConfigView::MachineConfigView(QWidget *parent) : QWidget(parent) {
  QGridLayout *layout = new QGridLayout();

  const int headerColumnSpan = 2;
  const int propertyValueColumn = 3;
  int rows = 0;

  layout->addWidget(new QLabel("<b>Hardware</b>"), rows++, 0, 1,
                    headerColumnSpan);

  layout->addWidget(new QLabel("Processors:"), rows, 1);
  numCpusLabel = new QLabel;
  layout->addWidget(numCpusLabel, rows++, propertyValueColumn);

  layout->addWidget(new QLabel("Clock rate:"), rows, 1);
  clockRateLabel = new QLabel;
  layout->addWidget(clockRateLabel, rows++, propertyValueColumn);

  layout->addWidget(new QLabel("TLB size:"), rows, 1);
  tlbSizeLabel = new QLabel;
  layout->addWidget(tlbSizeLabel, rows++, propertyValueColumn);

  layout->addWidget(new QLabel("RAM size:"), rows, 1);
  ramSizeLabel = new QLabel;
  layout->addWidget(ramSizeLabel, rows++, propertyValueColumn);

  layout->addWidget(new QLabel("RAMTOP value:"), rows, 1);
  ramtopLabel = new QLabel;
  layout->addWidget(ramtopLabel, rows++, propertyValueColumn);

  layout->addWidget(new QLabel("TLB Floor Address:"), rows, 1);
  tlbFloorAddressLabel = new QLabel;
  layout->addWidget(tlbFloorAddressLabel, rows++, propertyValueColumn);

  layout->addWidget(new QLabel("Byte order:"), rows, 1);
  QLabel *byteOrderLabel =
      new QLabel(BIGENDIANCPU ? "Big-endian" : "Little-endian");
  layout->addWidget(byteOrderLabel, rows++, propertyValueColumn);

  layout->setRowMinimumHeight(rows++, 10);

  layout->addWidget(new QLabel("<b>BIOS</b>"), rows++, 0, 1, headerColumnSpan);

  layout->addWidget(new QLabel("Bootstrap BIOS:"), rows, 1);
  bootstrapROMLabel = new QLabel;
  layout->addWidget(bootstrapROMLabel, rows++, propertyValueColumn);

  layout->addWidget(new QLabel("Execution BIOS:"), rows, 1);
  executionROMLabel = new QLabel;
  layout->addWidget(executionROMLabel, rows++, propertyValueColumn);

  layout->setRowMinimumHeight(rows++, 10);

  layout->addWidget(new QLabel("<b>Kernel Boot</b>"), rows++, 0, 1,
                    headerColumnSpan);

  layout->addWidget(new QLabel("Load core file:"), rows, 1);
  loadCoreLabel = new QLabel;
  layout->addWidget(loadCoreLabel, rows++, propertyValueColumn);

  layout->addWidget(new QLabel("Core file:"), rows, 1);
  coreFileLabel = new QLabel;
  layout->addWidget(coreFileLabel, rows++, propertyValueColumn);

  layout->setRowMinimumHeight(rows++, 10);

  layout->addWidget(new QLabel("<b>Debugging</b>"), rows++, 0, 1,
                    headerColumnSpan);

  layout->addWidget(new QLabel("Symbol table:"), rows, 1);
  stabLabel = new QLabel;
  layout->addWidget(stabLabel, rows++, propertyValueColumn);

  layout->addWidget(new QLabel("ASID:"), rows, 1);
  stabAsidLabel = new QLabel;
  layout->addWidget(stabAsidLabel, rows++, propertyValueColumn);

  layout->setColumnMinimumWidth(0, 10);
  layout->setColumnMinimumWidth(3, 70);
  layout->setColumnMinimumWidth(2, 35);

  layout->setColumnStretch(4, 1);
  layout->setRowStretch(rows, 1);

  setLayout(layout);

  Update();
}

void MachineConfigView::Update() {
  const MachineConfig *config = Appl()->getConfig();
  Word tlbFloorAddr = config->getTLBFloorAddress();
  Word ramtop = RAMBASE + (config->getRamSize() * FRAMESIZE * FRAMEKB);

  numCpusLabel->setNum((int)config->getNumProcessors());
  clockRateLabel->setText(QString("%1 MHz").arg(config->getClockRate()));
  tlbSizeLabel->setNum((int)config->getTLBSize());

  ramSizeLabel->setText(QString("%1 Frames").arg(config->getRamSize()));
  ramtopLabel->setText(FormatAddress(ramtop));

  if (tlbFloorAddr == ramtop)
    tlbFloorAddressLabel->setText("RAMTOP");
  else if (tlbFloorAddr == MAXWORDVAL)
    tlbFloorAddressLabel->setText("VM OFF");
  else
    tlbFloorAddressLabel->setText(FormatAddress(tlbFloorAddr));

  bootstrapROMLabel->setText(config->getROM(ROM_TYPE_BOOT).c_str());
  executionROMLabel->setText(config->getROM(ROM_TYPE_BIOS).c_str());

  loadCoreLabel->setText(config->isLoadCoreEnabled() ? "Yes" : "No");
  coreFileLabel->setText(
      checkedFileName(config->getROM(ROM_TYPE_CORE).c_str()));
  coreFileLabel->setEnabled(config->isLoadCoreEnabled());

  stabLabel->setText(config->getROM(ROM_TYPE_STAB).c_str());
  stabAsidLabel->setText(
      QString("0x%1").arg(config->getSymbolTableASID(), 2, 16, QChar('0')));
}

QString MachineConfigView::checkedFileName(const QString &fileName) {
  return fileName.isEmpty() ? "-" : fileName;
}
