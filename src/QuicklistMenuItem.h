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
#include <libdbusmenu-glib/client.h>

#include "Nux/Nux.h"
#include "Nux/View.h"

#include <pango/pango.h>
#include <pango/pangocairo.h>


void
GetDPI (int &dpi_x, int &dpi_y);

typedef enum
{
  FONTSTYLE_NORMAL  = PANGO_STYLE_NORMAL,
  FONTSTYLE_OBLIQUE = PANGO_STYLE_OBLIQUE,
  FONTSTYLE_ITALIC  = PANGO_STYLE_ITALIC,
}  FontStyle;

typedef enum
{
  FONTWEIGHT_LIGHT  = PANGO_WEIGHT_LIGHT,
  FONTWEIGHT_NORMAL = PANGO_WEIGHT_NORMAL,
  FONTWEIGHT_BOLD   = PANGO_WEIGHT_BOLD,
} FontWeight;


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

    sigc::signal<void, QuicklistMenuItem&> sigChanged;

    //! Return the size of the text + size of associated radio button or check box
    virtual void GetTextExtents (int &width, int &height);
    
    virtual void UpdateTexture ();
    void ItemActivated ();

    sigc::signal<void, QuicklistMenuItem*> sigTextChanged;
    sigc::signal<void, QuicklistMenuItem*> sigColorChanged;
    
  protected:
    
    nux::BaseTexture* _normalTexture[2];
    nux::BaseTexture* _activeTexture[2];
    nux::BaseTexture* _insensitiveTexture;
    DbusmenuMenuitem* _menuItem;
    nux::Color        _color;
    bool              _debug;


    void DrawRoundedRectangle ();
};

#endif // QUICKLISTMENUITEM_H
