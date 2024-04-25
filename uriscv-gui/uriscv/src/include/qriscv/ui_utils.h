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

#ifndef QRISCV_UI_UTILS_H
#define QRISCV_UI_UTILS_H

#include <QString>

#include "uriscv/types.h"

class SymbolTable;
class QTreeView;

const char *GetSymbolicAddress(const SymbolTable *symbolTable, Word asid,
                               Word address, bool onlyFunctions, SWord *offset);

QString FormatAddress(Word address);

void SetFirstColumnSpanned(QTreeView *treeView, bool setting = true);

#endif // QRISCV_UI_UTILS_H
