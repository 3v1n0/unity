// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#ifndef QUICKLISTMENUITEM_H
#define QUICKLISTMENUITEM_H

#include <libdbusmenu-glib/menuitem.h>
#include <libdbusmenu-glib/client.h>

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <NuxGraphics/CairoGraphics.h>

#include <pango/pango.h>
#include <pango/pangocairo.h>

#include "unity-shared/Introspectable.h"

namespace unity
{

enum QuicklistMenuItemType
{
  MENUITEM_TYPE_UNKNOWN = 0,
  MENUITEM_TYPE_LABEL,
  MENUITEM_TYPE_SEPARATOR,
  MENUITEM_TYPE_CHECK,
  MENUITEM_TYPE_RADIO,
};

class QuicklistMenuItem : public nux::View, public debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(QuicklistMenuItem, nux::View);
public:
  QuicklistMenuItem(DbusmenuMenuitem* item,
                    NUX_FILE_LINE_PROTO);

  QuicklistMenuItem(DbusmenuMenuitem* item,
                    bool              debug,
                    NUX_FILE_LINE_PROTO);

  virtual ~QuicklistMenuItem();

  void PreLayoutManagement();

  long PostLayoutManagement(long layoutResult);

  void Draw(nux::GraphicsEngine& gfxContext,
            bool                 forceDraw);

  void DrawContent(nux::GraphicsEngine& gfxContext,
                   bool                 forceDraw);

  void PostDraw(nux::GraphicsEngine& gfxContext,
                bool                 forceDraw);

  QuicklistMenuItemType GetItemType();

  void ItemActivated();
  void EnableLabelMarkup(bool enabled);
  bool IsMarkupEnabled();

  sigc::signal<void, QuicklistMenuItem&> sigChanged;
  sigc::signal<void, QuicklistMenuItem*> sigTextChanged;
  sigc::signal<void, QuicklistMenuItem*> sigColorChanged;

  virtual const gchar* GetLabel();
  virtual bool GetEnabled();
  virtual bool GetActive();
  virtual bool GetVisible();
  virtual bool GetSelectable();

protected:
  // Introspection
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  static const int ITEM_INDENT_ABS = 16;
  static const int ITEM_CORNER_RADIUS_ABS = 3;
  static const int ITEM_MARGIN = 4;

  std::string           _text;
  nux::Color            _textColor;
  int                   _pre_layout_width;
  int                   _pre_layout_height;
  nux::CairoGraphics*   _cairoGraphics;

  nux::BaseTexture*     _normalTexture[2];
  nux::BaseTexture*     _prelightTexture[2];

  void Initialize(DbusmenuMenuitem* item, bool debug);
  void InitializeText();
  virtual const gchar* GetDefaultText();

  gchar* GetText();
  //! Return the size of the text + size of associated radio button or check box
  void GetTextExtents(int& width, int& height);
  void GetTextExtents(const gchar* font, int& width, int& height);
  virtual void UpdateTexture() = 0;
  virtual int CairoSurfaceWidth() = 0;

  void RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);

  sigc::signal<void, QuicklistMenuItem*> sigMouseEnter;
  sigc::signal<void, QuicklistMenuItem*> sigMouseLeave;
  sigc::signal<void, QuicklistMenuItem*, int, int> sigMouseReleased;
  sigc::signal<void, QuicklistMenuItem*, int, int> sigMouseClick;
  sigc::signal<void, QuicklistMenuItem*, int, int> sigMouseDrag;

  DbusmenuMenuitem* _menuItem;
  QuicklistMenuItemType _item_type;

  nux::Color        _color;   //!< Item rendering color factor.
  bool              _debug;

  bool _prelight;   //!< True when the mouse is over the item.

  void DrawText(nux::CairoGraphics* cairo, int width, int height, nux::Color const& color);
  void DrawPrelight(nux::CairoGraphics* cairo, int width, int height, nux::Color const& color);

  // Introspection
  std::string _name;

  friend class QuicklistView;
};

} // NAMESPACE

#endif // QUICKLISTMENUITEM_H
