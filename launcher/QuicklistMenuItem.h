// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 *              Jay Taoko <jay.taoko@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef QUICKLISTMENUITEM_H
#define QUICKLISTMENUITEM_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <NuxGraphics/CairoGraphics.h>
#include <libdbusmenu-glib/menuitem.h>
#include <UnityCore/GLibWrapper.h>

#include <pango/pango.h>
#include <pango/pangocairo.h>

#include "unity-shared/Introspectable.h"

namespace unity
{

enum class QuicklistMenuItemType
{
  UNKNOWN = 0,
  LABEL,
  SEPARATOR,
  CHECK,
  RADIO
};

class QuicklistMenuItem : public nux::View, public debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(QuicklistMenuItem, nux::View);
public:
  QuicklistMenuItem(QuicklistMenuItemType type, glib::Object<DbusmenuMenuitem> const& item, NUX_FILE_LINE_PROTO);
  virtual ~QuicklistMenuItem();

  QuicklistMenuItemType GetItemType() const;
  virtual std::string GetLabel() const;
  virtual bool GetEnabled() const;
  virtual bool GetActive() const;
  virtual bool GetVisible() const;
  virtual bool GetSelectable() const;

  void EnableLabelMarkup(bool enabled);
  bool IsMarkupEnabled() const;
  void EnableLabelMarkupAccel(bool enabled);
  bool IsMarkupAccelEnabled() const;

  void SetMaxLabelWidth(int max_width);
  int GetMaxLabelWidth() const;

  bool IsOverlayQuicklist() const;

  void Activate() const;

  void Select(bool select = true);
  bool IsSelected() const;

  nux::Size const& GetTextExtents() const;
  virtual void UpdateTexture() = 0;
  unsigned GetCairoSurfaceWidth() const;

  sigc::signal<void, QuicklistMenuItem*> sigTextChanged;
  sigc::signal<void, QuicklistMenuItem*> sigColorChanged;
  sigc::signal<void, QuicklistMenuItem*> sigMouseEnter;
  sigc::signal<void, QuicklistMenuItem*> sigMouseLeave;
  sigc::signal<void, QuicklistMenuItem*, int, int> sigMouseReleased;
  sigc::signal<void, QuicklistMenuItem*, int, int> sigMouseClick;
  sigc::signal<void, QuicklistMenuItem*, int, int> sigMouseDrag;

  static const char* MARKUP_ENABLED_PROPERTY;
  static const char* MARKUP_ACCEL_DISABLED_PROPERTY;
  static const char* MAXIMUM_LABEL_WIDTH_PROPERTY;
  static const char* OVERLAY_MENU_ITEM_PROPERTY;
  static const char* QUIT_ACTION_PROPERTY;

protected:
  // Introspection
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  static const int ITEM_INDENT_ABS = 16;
  static const int ITEM_CORNER_RADIUS_ABS = 3;
  static const int ITEM_MARGIN = 4;

  void InitializeText();

  virtual std::string GetDefaultText() const;
  std::string GetText() const;

  static double Align(double val);

  void RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);

  void PreLayoutManagement();
  long PostLayoutManagement(long layoutResult);
  void Draw(nux::GraphicsEngine& gfxContext, bool forceDraw);
  void DrawText(nux::CairoGraphics& cairo, int width, int height, nux::Color const& color);
  void DrawPrelight(nux::CairoGraphics& cairo, int width, int height, nux::Color const& color);

  nux::ObjectPtr<nux::BaseTexture> _normalTexture[2];
  nux::ObjectPtr<nux::BaseTexture> _prelightTexture[2];
  QuicklistMenuItemType _item_type;
  glib::Object<DbusmenuMenuitem> _menu_item;
  mutable Time _activate_timestamp;
  bool _prelight;
  int _pre_layout_width;
  int _pre_layout_height;
  nux::Size _text_extents;
  std::string _text;
};

} // NAMESPACE

#endif // QUICKLISTMENUITEM_H
