/*
 * Copyright (C) 2010 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 */

#include "Nux/Nux.h"
#include "QuicklistMenuItemRadio.h"

QuicklistMenuItemRadio::QuicklistMenuItemRadio (DbusmenuMenuitem* item,
                                                NUX_FILE_LINE_DECL) :
QuicklistMenuItem (item,
                   NUX_FILE_LINE_PARAM)
{
}

QuicklistMenuItemRadio::QuicklistMenuItemRadio (DbusmenuMenuitem* item,
                                                bool              debug,
                                                NUX_FILE_LINE_DECL) :
QuicklistMenuItem (item,
                   debug,
                   NUX_FILE_LINE_PARAM)
{
}

QuicklistMenuItemRadio::~QuicklistMenuItemRadio ()
{
}

void
QuicklistMenuItemRadio::DrawText ()
{
}

void
QuicklistMenuItemRadio::GetExtents ()
{
}

void
QuicklistMenuItemRadio::UpdateTextures ()
{
}

void
QuicklistMenuItemRadio::DrawRoundedRectangle ()
{
}
