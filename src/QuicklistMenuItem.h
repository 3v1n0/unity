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

typedef enum
{
  MENUITEM_TYPE_UNKNOWN    = 0,
  MENUITEM_TYPE_LABEL,
  MENUITEM_TYPE_SEPARATOR,
  MENUITEM_TYPE_CHECK,
  MENUITEM_TYPE_RADIO,
} QuicklistMenuItemType;

class QuicklistMenuItem : public nux::View
{
  public:
    QuicklistMenuItem (DbusmenuMenuitem* item,
                       NUX_FILE_LINE_PROTO);

    QuicklistMenuItem (DbusmenuMenuitem* item,
                       bool              debug,
                       NUX_FILE_LINE_PROTO);

    virtual ~QuicklistMenuItem ();

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

    QuicklistMenuItemType GetItemType ();
    
    //! Return the size of the text + size of associated radio button or check box
    virtual void GetTextExtents (int &width, int &height);
    
    virtual void UpdateTexture ();
    void ItemActivated ();

    sigc::signal<void, QuicklistMenuItem&> sigChanged;
    sigc::signal<void, QuicklistMenuItem*> sigTextChanged;
    sigc::signal<void, QuicklistMenuItem*> sigColorChanged;
    
    
  protected:

    void RecvMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags);
    void RecvMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags);
    void RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags);
    void RecvMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags);
    void RecvMouseClick (int x, int y, unsigned long button_flags, unsigned long key_flags);
    void RecvMouseMove (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
    void RecvMouseDrag (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
    
    sigc::signal<void, QuicklistMenuItem*> sigMouseEnter;
    sigc::signal<void, QuicklistMenuItem*> sigMouseLeave;
    sigc::signal<void, QuicklistMenuItem*> sigMouseReleased;
    sigc::signal<void, QuicklistMenuItem*> sigMouseClick;
    
    nux::BaseTexture* _normalTexture[2];
    nux::BaseTexture* _activeTexture[2];
    nux::BaseTexture* _insensitiveTexture;
    DbusmenuMenuitem* _menuItem;
    nux::Color        _color;
    bool              _debug;
    QuicklistMenuItemType _item_type;
    
    bool _enabled;
    bool _active;
    bool _prelight;

    void DrawRoundedRectangle ();
    
    friend class QuicklistView;
};

#endif // QUICKLISTMENUITEM_H
