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
#include "QuicklistMenuItemCheckmark.h"

QuicklistMenuItemCheckmark::QuicklistMenuItemCheckmark (DbusmenuMenuitem* item,
                                                        NUX_FILE_LINE_DECL) :
QuicklistMenuItem (item,
                   NUX_FILE_LINE_PARAM)
{
}

QuicklistMenuItemCheckmark::QuicklistMenuItemCheckmark (DbusmenuMenuitem* item,
                                                        bool              debug,
                                                        NUX_FILE_LINE_DECL) :
QuicklistMenuItem (item,
                   debug,
                   NUX_FILE_LINE_PARAM)
{
}

QuicklistMenuItemCheckmark::~QuicklistMenuItemCheckmark ()
{
}

void
QuicklistMenuItemCheckmark::PreLayoutManagement ()
{
}

long
QuicklistMenuItemCheckmark::PostLayoutManagement (long layoutResult)
{
  long result = View::PostLayoutManagement (layoutResult);

  return result;
}

long
QuicklistMenuItemCheckmark::ProcessEvent (nux::IEvent& event,
                                          long         traverseInfo,
                                          long         processEventInfo)
{
  long result = traverseInfo;

  result = nux::View::PostProcessEvent2 (event, result, processEventInfo);
  return result;

}

void
QuicklistMenuItemCheckmark::Draw (nux::GraphicsEngine& gfxContext,
                                  bool                 forceDraw)
{
}

void
QuicklistMenuItemCheckmark::DrawContent (nux::GraphicsEngine& gfxContext,
                                         bool                 forceDraw)
{
}

void
QuicklistMenuItemCheckmark::PostDraw (nux::GraphicsEngine& gfxContext,
                                      bool                 forceDraw)
{
}

void
QuicklistMenuItemCheckmark::DrawText ()
{
}

void
QuicklistMenuItemCheckmark::GetExtents ()
{
}

void
QuicklistMenuItemCheckmark::UpdateTextures ()
{
}

void
QuicklistMenuItemCheckmark::DrawRoundedRectangle ()
{
}
