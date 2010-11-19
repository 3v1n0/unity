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

#ifndef QUICKLISTMENUITEMSEPARATOR_H
#define QUICKLISTMENUITEMSEPARATOR_H

#include "Nux/Nux.h"
#include "Nux/View.h"
#include "NuxImage/CairoGraphics.h"
#include "QuicklistMenuItem.h"

#if defined(NUX_OS_LINUX)
#include <X11/Xlib.h>
#endif

class QuicklistMenuItemSeparator : public QuicklistMenuItem
{
  public:
    QuicklistMenuItemSeparator (DbusmenuMenuitem* item,
                                NUX_FILE_LINE_PROTO);

    QuicklistMenuItemSeparator (DbusmenuMenuitem* item,
                                bool              debug,
                                NUX_FILE_LINE_PROTO);

    ~QuicklistMenuItemSeparator ();

    void PreLayoutManagement ();

    long PostLayoutManagement (long layoutResult);

    long ProcessEvent (nux::IEvent& event,
                       long         traverseInfo,
                       long         processEventInfo);

    void Draw (nux::GraphicsEngine& gfxContext,
               bool                 forceDraw);

    void DrawContent (nux::GraphicsEngine& gfxContext,
                      bool                 forceDraw);

    void PostDraw (nux::GraphicsEngine& gfxContext,
                   bool                 forceDraw);

  private:
    int                 _pre_layout_width;
    int                 _pre_layout_height;
    nux::BaseTexture*   _normalTexture;
    nux::CairoGraphics* _cairoGraphics;

    void UpdateTexture ();
};

#endif // QUICKLISTMENUITEMSEPARATOR_H
