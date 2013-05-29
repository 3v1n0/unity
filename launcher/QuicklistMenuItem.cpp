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

#include <gtk/gtk.h>
#include <UnityCore/Variant.h>
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/UBusMessages.h"

#include "QuicklistMenuItem.h"

namespace unity
{
const char* QuicklistMenuItem::MARKUP_ENABLED_PROPERTY = "unity-use-markup";
const char* QuicklistMenuItem::MARKUP_ACCEL_DISABLED_PROPERTY = "unity-disable-accel";
const char* QuicklistMenuItem::MAXIMUM_LABEL_WIDTH_PROPERTY = "unity-max-label-width";
const char* QuicklistMenuItem::OVERLAY_MENU_ITEM_PROPERTY = "unity-overlay-item";

NUX_IMPLEMENT_OBJECT_TYPE(QuicklistMenuItem);

QuicklistMenuItem::QuicklistMenuItem(QuicklistMenuItemType type, glib::Object<DbusmenuMenuitem> const& item, NUX_FILE_LINE_DECL)
  : nux::View(NUX_FILE_LINE_PARAM)
  , _item_type(type)
  , _menu_item(item)
  , _activate_timestamp(0)
  , _prelight(false)
{
  mouse_up.connect(sigc::mem_fun(this, &QuicklistMenuItem::RecvMouseUp));
  mouse_click.connect(sigc::mem_fun(this, &QuicklistMenuItem::RecvMouseClick));
  mouse_drag.connect(sigc::mem_fun(this, &QuicklistMenuItem::RecvMouseDrag));
  mouse_enter.connect(sigc::mem_fun(this, &QuicklistMenuItem::RecvMouseEnter));
  mouse_leave.connect(sigc::mem_fun(this, &QuicklistMenuItem::RecvMouseLeave));
}

QuicklistMenuItem::~QuicklistMenuItem()
{}

std::string QuicklistMenuItem::GetDefaultText() const
{
  return "";
}

void QuicklistMenuItem::InitializeText()
{
  if (_menu_item)
    _text = GetText();
  else
    _text = GetDefaultText();

  // This is needed to setup the item size values
  nux::CairoGraphics cairoGraphics(CAIRO_FORMAT_A1, 1, 1);
  DrawText(cairoGraphics, 1, 1, nux::color::White);
}

QuicklistMenuItemType QuicklistMenuItem::GetItemType() const
{
  return _item_type;
}

std::string QuicklistMenuItem::GetLabel() const
{
  if (!_menu_item)
    return "";

  const char *label = dbusmenu_menuitem_property_get(_menu_item, DBUSMENU_MENUITEM_PROP_LABEL);

  return label ? label : "";
}

bool QuicklistMenuItem::GetEnabled() const
{
  if (!_menu_item)
    return false;

  return dbusmenu_menuitem_property_get_bool(_menu_item, DBUSMENU_MENUITEM_PROP_ENABLED);
}

bool QuicklistMenuItem::GetActive() const
{
  if (!_menu_item)
    return false;

  int toggle = dbusmenu_menuitem_property_get_int(_menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE);

  return (toggle == DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED);
}

bool QuicklistMenuItem::GetVisible() const
{
  if (!_menu_item)
    return false;

  return dbusmenu_menuitem_property_get_bool(_menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE);
}

bool QuicklistMenuItem::GetSelectable() const
{
  return GetVisible() && GetEnabled();
}

std::string QuicklistMenuItem::GetText() const
{
  std::string const& label = GetLabel();

  if (label.empty())
    return "";

  if (!IsMarkupEnabled())
  {
    return glib::String(g_markup_escape_text(label.c_str(), -1)).Str();
  }

  return label;
}

void QuicklistMenuItem::Activate() const
{
  if (!_menu_item || !GetSelectable())
    return;

  _activate_timestamp = nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp;
  dbusmenu_menuitem_handle_event(_menu_item, "clicked", nullptr, _activate_timestamp);

  if (!IsOverlayQuicklist())
  {
    UBusManager manager;
    manager.SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);
  }
}

void QuicklistMenuItem::Select(bool select)
{
  _prelight = select;
}

bool QuicklistMenuItem::IsSelected() const
{
  return _prelight;
}

void QuicklistMenuItem::RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  sigMouseReleased.emit(this, x, y);
}

void QuicklistMenuItem::RecvMouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  if (!GetEnabled())
    return;

  sigMouseClick.emit(this, x, y);
}

void QuicklistMenuItem::RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  sigMouseDrag.emit(this, x, y);
}

void QuicklistMenuItem::RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  sigMouseEnter.emit(this);
}

void QuicklistMenuItem::RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  sigMouseLeave.emit(this);
}

void QuicklistMenuItem::PreLayoutManagement()
{
  _pre_layout_width = GetBaseWidth();
  _pre_layout_height = GetBaseHeight();

  if (!_normalTexture[0])
  {
    UpdateTexture();
  }

  View::PreLayoutManagement();
}

long QuicklistMenuItem::PostLayoutManagement(long layoutResult)
{
  int w = GetBaseWidth();
  int h = GetBaseHeight();

  long result = 0;

  if (_pre_layout_width < w)
    result |= nux::SIZE_LARGER_WIDTH;
  else if (_pre_layout_width > w)
    result |= nux::SIZE_SMALLER_WIDTH;
  else
    result |= nux::SIZE_EQUAL_WIDTH;

  if (_pre_layout_height < h)
    result |= nux::SIZE_LARGER_HEIGHT;
  else if (_pre_layout_height > h)
    result |= nux::SIZE_SMALLER_HEIGHT;
  else
    result |= nux::SIZE_EQUAL_HEIGHT;

  return result;
}

void QuicklistMenuItem::Draw(nux::GraphicsEngine& gfxContext, bool forceDraw)
{
  // Check if the texture have been computed. If they haven't, exit the function.
  if (!_normalTexture[0] || !_prelightTexture[0])
    return;

  nux::Geometry const& base = GetGeometry();

  gfxContext.PushClippingRectangle(base);

  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

  gfxContext.GetRenderStates().SetBlend(true);
  gfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  nux::ObjectPtr<nux::IOpenGLBaseTexture> texture;
  unsigned int texture_idx = GetActive() ? 1 : 0;
  bool enabled = GetEnabled();

  if (!_prelight || !enabled)
  {
    texture = _normalTexture[texture_idx]->GetDeviceTexture();
  }
  else
  {
    texture = _prelightTexture[texture_idx]->GetDeviceTexture();
  }

  nux::Color const& color = enabled ? nux::color::White : nux::color::White * 0.35;

  gfxContext.QRP_1Tex(base.x, base.y, base.width, base.height, texture, texxform, color);
  gfxContext.GetRenderStates().SetBlend(false);
  gfxContext.PopClippingRectangle();
}

nux::Size const& QuicklistMenuItem::GetTextExtents() const
{
  return _text_extents;
}

void QuicklistMenuItem::DrawText(nux::CairoGraphics& cairo, int width, int height, nux::Color const& color)
{
  if (_text.empty())
    return;

  GdkScreen* screen = gdk_screen_get_default(); // not ref'ed
  GtkSettings* settings = gtk_settings_get_default(); // not ref'ed

  glib::String font_name;
  g_object_get(settings, "gtk-font-name", &font_name, nullptr);

  std::shared_ptr<cairo_t> cairo_context(cairo.GetContext(), cairo_destroy);
  cairo_t* cr = cairo_context.get();
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba(cr, color.red, color.blue, color.green, color.alpha);
  cairo_set_font_options(cr, gdk_screen_get_font_options(screen));

  glib::Object<PangoLayout> layout(pango_cairo_create_layout(cr));
  std::shared_ptr<PangoFontDescription> desc(pango_font_description_from_string(font_name), pango_font_description_free);
  pango_layout_set_font_description(layout, desc.get());
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

  if (IsMarkupAccelEnabled())
    pango_layout_set_markup_with_accel(layout, _text.c_str(), -1, '_', nullptr);
  else
    pango_layout_set_markup(layout, _text.c_str(), -1);

  if (GetMaxLabelWidth() > 0)
  {
    int max_width = std::min<int>(GetMaximumWidth(), GetMaxLabelWidth());
    pango_layout_set_width(layout, max_width * PANGO_SCALE);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
  }

  PangoContext* pangoCtx = pango_layout_get_context(layout);  // is not ref'ed
  pango_cairo_context_set_font_options(pangoCtx, gdk_screen_get_font_options(screen));

  int dpi = 0;
  g_object_get(settings, "gtk-xft-dpi", &dpi, nullptr);

  if (dpi == -1)
  {
    // use some default DPI-value
    pango_cairo_context_set_resolution(pangoCtx, 96.0f);
  }
  else
  {
    pango_cairo_context_set_resolution(pangoCtx, static_cast<float>(dpi) / static_cast<float>(PANGO_SCALE));
  }

  pango_layout_context_changed(layout);
  PangoRectangle log_rect  = {0, 0, 0, 0};
  pango_layout_get_extents(layout, nullptr, &log_rect);

  int text_width  = log_rect.width / PANGO_SCALE;
  int text_height = log_rect.height / PANGO_SCALE;

  _text_extents.width = text_width + ITEM_INDENT_ABS + 3 * ITEM_MARGIN;
  _text_extents.height = text_height + 2 * ITEM_MARGIN;

  SetMinimumSize(_text_extents.width, _text_extents.height);

  cairo_move_to(cr, 2 * ITEM_MARGIN + ITEM_INDENT_ABS, static_cast<float>(height - text_height) / 2.0f);
  pango_cairo_show_layout(cr, layout);
}

void QuicklistMenuItem::DrawPrelight(nux::CairoGraphics& cairo, int width, int height, nux::Color const& color)
{
  std::shared_ptr<cairo_t> cairo_context(cairo.GetContext(), cairo_destroy);
  cairo_t* cr = cairo_context.get();

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba(cr, color.red, color.blue, color.green, color.alpha);
  cairo.DrawRoundedRectangle(cr, 1.0f, 0.0f, 0.0f, ITEM_CORNER_RADIUS_ABS, width, height);
  cairo_fill(cr);
}

double QuicklistMenuItem::Align(double val)
{
  const double fract = val - static_cast<int>(val);

  if (fract != 0.5f)
    return static_cast<double>(static_cast<int>(val) + 0.5f);
  else
    return val;
}

void QuicklistMenuItem::EnableLabelMarkup(bool enabled)
{
  if (IsMarkupEnabled() != enabled)
  {
    dbusmenu_menuitem_property_set_bool(_menu_item, MARKUP_ENABLED_PROPERTY, enabled ? TRUE : FALSE);
    InitializeText();
  }
}

bool QuicklistMenuItem::IsMarkupEnabled() const
{
  if (!_menu_item)
    return false;

  gboolean markup = dbusmenu_menuitem_property_get_bool(_menu_item, MARKUP_ENABLED_PROPERTY);
  return (markup != FALSE);
}

void QuicklistMenuItem::EnableLabelMarkupAccel(bool enabled)
{
  if (IsMarkupAccelEnabled() != enabled)
  {
    dbusmenu_menuitem_property_set_bool(_menu_item, MARKUP_ACCEL_DISABLED_PROPERTY, enabled ? FALSE : TRUE);
    InitializeText();
  }
}

bool QuicklistMenuItem::IsMarkupAccelEnabled() const
{
  if (!_menu_item)
    return false;

  gboolean disabled = dbusmenu_menuitem_property_get_bool(_menu_item, MARKUP_ACCEL_DISABLED_PROPERTY);
  return (disabled == FALSE);
}

void QuicklistMenuItem::SetMaxLabelWidth(int max_width)
{
  if (GetMaxLabelWidth() != max_width)
  {
    dbusmenu_menuitem_property_set_int(_menu_item, MAXIMUM_LABEL_WIDTH_PROPERTY, max_width);
    InitializeText();
  }
}

int QuicklistMenuItem::GetMaxLabelWidth() const
{
  if (!_menu_item)
    return -1;

  return dbusmenu_menuitem_property_get_int(_menu_item, MAXIMUM_LABEL_WIDTH_PROPERTY);
}

bool QuicklistMenuItem::IsOverlayQuicklist() const
{
  if (!_menu_item)
    return false;

  gboolean overlay = dbusmenu_menuitem_property_get_bool(_menu_item, OVERLAY_MENU_ITEM_PROPERTY);
  return (overlay != FALSE);
}

unsigned QuicklistMenuItem::GetCairoSurfaceWidth() const
{
  if (!_normalTexture[0])
    return 0;

  return _normalTexture[0]->GetWidth();
}

// Introspection

std::string QuicklistMenuItem::GetName() const
{
  return "QuicklistMenuItem";
}

void QuicklistMenuItem::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder)
  .add(GetAbsoluteGeometry())
  .add("text", _text)
  .add("enabled", GetEnabled())
  .add("active", GetActive())
  .add("visible", GetVisible())
  .add("selectable", GetSelectable())
  .add("selected", _prelight)
  .add("activate_timestamp", (uint32_t) _activate_timestamp);
}

} //NAMESPACE
