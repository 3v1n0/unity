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

#ifndef QUICKLISTMENUITEM_H
#define QUICKLISTMENUITEM_H

#include <libdbusmenu-glib/menuitem.h>

#include "Nux/Nux.h"
#include "Nux/View.h"

#if defined(NUX_OS_LINUX)
#include <X11/Xlib.h>
#endif

class QuicklistMenuItem : public nux::View
{
  public:
    QuicklistMenuItem (DbusmenuMenuitem* item,
                       NUX_FILE_LINE_PROTO);

    QuicklistMenuItem (DbusmenuMenuitem* item,
                       bool              debug,
                       NUX_FILE_LINE_PROTO);

    ~QuicklistMenuItem ();

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

    // public API
    virtual const gchar* GetLabel ();

    virtual bool         GetEnabled ();

    virtual bool         GetActive ();

    virtual void         SetColor (nux::Color color);

    //sigc::signal<void, QuicklistMenuItem&> sigColorChanged;

    void UpdateTextures ();

    void ItemActivated ();

  private:
    nux::BaseTexture* _normalTexture[2];
    nux::BaseTexture* _activeTexture[2];
    nux::BaseTexture* _insensitiveTexture;
    DbusmenuMenuitem* _menuItem;
    bool              _debug;
    nux::Color        _color;

    void DrawText ();

    void GetExtents ();

    void DrawRoundedRectangle ();
};

#endif // QUICKLISTMENUITEM_H
