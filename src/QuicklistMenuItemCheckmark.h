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
 * Authored by: Jay Taoko <jay.taoko@canonical.com>
 */

#ifndef QUICKLISTMENUITEMCHECKMARK_H
#define QUICKLISTMENUITEMCHECKMARK_H

#include "Nux/Nux.h"
#include "Nux/View.h"
#include "NuxImage/CairoGraphics.h"

#include "QuicklistMenuItem.h"

#if defined(NUX_OS_LINUX)
#include <X11/Xlib.h>
#endif

class QuicklistMenuItemCheckmark : public QuicklistMenuItem
{
  public:
    QuicklistMenuItemCheckmark (DbusmenuMenuitem* item,
                                NUX_FILE_LINE_PROTO);

    QuicklistMenuItemCheckmark (DbusmenuMenuitem* item,
                                bool              debug,
                                NUX_FILE_LINE_PROTO);

    ~QuicklistMenuItemCheckmark ();
    
  protected:
    
    void PreLayoutManagement ();

    long PostLayoutManagement (long layoutResult);

    long ProcessEvent (nux::IEvent& event, long traverseInfo, long processEventInfo);

    void Draw (nux::GraphicsEngine& gfxContext, bool forceDraw);

    void DrawContent (nux::GraphicsEngine& gfxContext, bool forceDraw);

    void PostDraw (nux::GraphicsEngine& gfxContext, bool forceDraw);
    
    nux::BaseTexture* _normalTexture[2];
    nux::BaseTexture* _prelightTexture[2];

    void Initialize (DbusmenuMenuitem* item);
    void DrawText (cairo_t*   cr,
                   int        width,
                   int        height,
                   nux::Color color);

    virtual void UpdateTexture ();
    virtual int CairoSurfaceWidth ();
    void DrawRoundedRectangle (cairo_t* cr,
                               double   aspect,
                               double   x,
                               double   y,
                               double   cornerRadius,
                               double   width,
                               double   height);
};

#endif // QUICKLISTMENUITEMCHECKMARK_H
