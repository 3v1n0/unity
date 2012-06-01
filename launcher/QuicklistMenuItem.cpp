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
 *              Jay Taoko <jay.taoko@canonical.com>
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <Nux/Nux.h>

#include "QuicklistMenuItem.h"
#include <UnityCore/Variant.h>

#include <X11/Xlib.h>

namespace unity
{

NUX_IMPLEMENT_OBJECT_TYPE(QuicklistMenuItem);

static void
OnPropertyChanged(gchar*             property,
                  GValue*            value,
                  QuicklistMenuItem* self);

static void
OnItemActivated(guint              timestamp,
                QuicklistMenuItem* self);

QuicklistMenuItem::QuicklistMenuItem(DbusmenuMenuitem* item,
                                     NUX_FILE_LINE_DECL) :
  View(NUX_FILE_LINE_PARAM)
{
  if (item == 0)
  {
    g_warning("Invalid DbusmenuMenuitem in file %s at line %s.", G_STRFUNC, G_STRLOC);
  }

  Initialize(item, false);
}

QuicklistMenuItem::QuicklistMenuItem(DbusmenuMenuitem* item,
                                     bool              debug,
                                     NUX_FILE_LINE_DECL) :
  View(NUX_FILE_LINE_PARAM)
{
  Initialize(item, debug);
}

void
QuicklistMenuItem::Initialize(DbusmenuMenuitem* item, bool debug)
{
  _text        = "";
  _color       = nux::Color(1.0f, 1.0f, 1.0f, 1.0f);
  _menuItem    = DBUSMENU_MENUITEM(g_object_ref(item));
  _debug       = debug;
  _item_type   = MENUITEM_TYPE_UNKNOWN;

  _normalTexture[0]   = NULL;
  _normalTexture[1]   = NULL;
  _prelightTexture[0] = NULL;
  _prelightTexture[1] = NULL;

  if (_menuItem)
  {
    g_signal_connect(_menuItem,
                     "property-changed",
                     G_CALLBACK(OnPropertyChanged),
                     this);
    g_signal_connect(_menuItem,
                     "item-activated",
                     G_CALLBACK(OnItemActivated),
                     this);
  }

  mouse_up.connect(sigc::mem_fun(this, &QuicklistMenuItem::RecvMouseUp));
  mouse_click.connect(sigc::mem_fun(this, &QuicklistMenuItem::RecvMouseClick));
  mouse_drag.connect(sigc::mem_fun(this, &QuicklistMenuItem::RecvMouseDrag));
  mouse_enter.connect(sigc::mem_fun(this, &QuicklistMenuItem::RecvMouseEnter));
  mouse_leave.connect(sigc::mem_fun(this, &QuicklistMenuItem::RecvMouseLeave));

  _prelight = false;
}

QuicklistMenuItem::~QuicklistMenuItem()
{
  if (_normalTexture[0])
    _normalTexture[0]->UnReference();

  if (_normalTexture[1])
    _normalTexture[1]->UnReference();

  if (_prelightTexture[0])
    _prelightTexture[0]->UnReference();

  if (_prelightTexture[1])
    _prelightTexture[1]->UnReference();

  if (_menuItem)
    g_object_unref(_menuItem);
}

const gchar*
QuicklistMenuItem::GetDefaultText()
{
  return NULL;
}

void
QuicklistMenuItem::InitializeText()
{
  if (_menuItem)
    _text = GetText();
  else
    _text = GetDefaultText();

  int textWidth = 1;
  int textHeight = 1;
  GetTextExtents(textWidth, textHeight);
  SetMinimumSize(textWidth + ITEM_INDENT_ABS + 3 * ITEM_MARGIN,
                 textHeight + 2 * ITEM_MARGIN);
}

QuicklistMenuItemType QuicklistMenuItem::GetItemType()
{
  return _item_type;
}

void
QuicklistMenuItem::PreLayoutManagement()
{
  View::PreLayoutManagement();
}

long
QuicklistMenuItem::PostLayoutManagement(long layoutResult)
{
  long result = View::PostLayoutManagement(layoutResult);

  return result;
}

void
QuicklistMenuItem::Draw(nux::GraphicsEngine& gfxContext,
                        bool                 forceDraw)
{
}

void
QuicklistMenuItem::DrawContent(nux::GraphicsEngine& gfxContext,
                               bool                 forceDraw)
{
}

void
QuicklistMenuItem::PostDraw(nux::GraphicsEngine& gfxContext,
                            bool                 forceDraw)
{
}

const gchar*
QuicklistMenuItem::GetLabel()
{
  if (_menuItem == 0)
    return 0;
  return dbusmenu_menuitem_property_get(_menuItem,
                                        DBUSMENU_MENUITEM_PROP_LABEL);
}

bool
QuicklistMenuItem::GetEnabled()
{
  if (_menuItem == 0)
    return false;
  return dbusmenu_menuitem_property_get_bool(_menuItem,
                                             DBUSMENU_MENUITEM_PROP_ENABLED);
}

bool
QuicklistMenuItem::GetActive()
{
  if (_menuItem == 0)
    return false;
  return dbusmenu_menuitem_property_get_int(_menuItem,
                                            DBUSMENU_MENUITEM_PROP_TOGGLE_STATE) == DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED;
}

bool
QuicklistMenuItem::GetVisible()
{
  if (_menuItem == 0)
    return false;
  return dbusmenu_menuitem_property_get_bool(_menuItem,
                                             DBUSMENU_MENUITEM_PROP_VISIBLE);
}

bool
QuicklistMenuItem::GetSelectable()
{
  return GetVisible() && GetEnabled();
}

void QuicklistMenuItem::ItemActivated()
{
  if (_debug)
    sigChanged.emit(*this);

  std::cout << "ItemActivated() called" << std::endl;
}

gchar* QuicklistMenuItem::GetText()
{
  const gchar *label;
  gchar *text;

  if (!_menuItem)
    return NULL;

  label = GetLabel();

  if (!label)
    return NULL;

  if (!IsMarkupEnabled())
  {
    text = g_markup_escape_text(label, -1);
  }
  else
  {
    text = g_strdup(label);
  }

  return text;
}

void QuicklistMenuItem::GetTextExtents(int& width, int& height)
{
  GtkSettings* settings = gtk_settings_get_default();  // not ref'ed
  gchar*       fontName = NULL;

  g_object_get(settings, "gtk-font-name", &fontName, NULL);
  GetTextExtents(fontName, width, height);
  g_free(fontName);
}

void QuicklistMenuItem::GetTextExtents(const gchar* font,
                                       int&         width,
                                       int&         height)
{
  cairo_surface_t*      surface  = NULL;
  cairo_t*              cr       = NULL;
  PangoLayout*          layout   = NULL;
  PangoFontDescription* desc     = NULL;
  PangoContext*         pangoCtx = NULL;
  PangoRectangle        logRect  = {0, 0, 0, 0};
  int                   dpi      = 0;
  GdkScreen*            screen   = gdk_screen_get_default();    // is not ref'ed
  GtkSettings*          settings = gtk_settings_get_default();  // is not ref'ed

  // sanity check
  if (!font)
    return;

  if (_text == "")
    return;

  surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1);
  cr = cairo_create(surface);
  cairo_set_font_options(cr, gdk_screen_get_font_options(screen));
  layout = pango_cairo_create_layout(cr);
  desc = pango_font_description_from_string(font);
  pango_font_description_set_weight(desc, PANGO_WEIGHT_NORMAL);
  pango_layout_set_font_description(layout, desc);
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
  pango_layout_set_markup_with_accel(layout, _text.c_str(), -1, '_', NULL);
  pangoCtx = pango_layout_get_context(layout);  // is not ref'ed
  pango_cairo_context_set_font_options(pangoCtx,
                                       gdk_screen_get_font_options(screen));
  g_object_get(settings, "gtk-xft-dpi", &dpi, NULL);
  if (dpi == -1)
  {
    // use some default DPI-value
    pango_cairo_context_set_resolution(pangoCtx, 96.0f);
  }
  else
  {
    pango_cairo_context_set_resolution(pangoCtx,
                                       (float) dpi / (float) PANGO_SCALE);
  }
  pango_layout_context_changed(layout);
  pango_layout_get_extents(layout, NULL, &logRect);

  width  = logRect.width / PANGO_SCALE;
  height = logRect.height / PANGO_SCALE;

  // clean up
  pango_font_description_free(desc);
  g_object_unref(layout);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
}

static void
OnPropertyChanged(gchar*             property,
                  GValue*            value,
                  QuicklistMenuItem* self)
{
  //todo
  //self->UpdateTexture ();
}

static void
OnItemActivated(guint              timestamp,
                QuicklistMenuItem* self)
{
  //todo:
  //self->ItemActivated ();
}

void QuicklistMenuItem::RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  sigMouseReleased.emit(this, x, y);
}

void QuicklistMenuItem::RecvMouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  if (!GetEnabled())
  {
    return;
  }
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

void QuicklistMenuItem::DrawText(nux::CairoGraphics* cairo, int width, int height, nux::Color const& color)
{
  if (_text == "" || cairo == nullptr)
    return;

  cairo_t*              cr         = cairo->GetContext();
  int                   textWidth  = 0;
  int                   textHeight = 0;
  PangoLayout*          layout     = NULL;
  PangoFontDescription* desc       = NULL;
  PangoContext*         pangoCtx   = NULL;
  int                   dpi        = 0;
  GdkScreen*            screen     = gdk_screen_get_default();    // not ref'ed
  GtkSettings*          settings   = gtk_settings_get_default();  // not ref'ed
  gchar*                fontName   = NULL;

  g_object_get(settings, "gtk-font-name", &fontName, NULL);
  GetTextExtents(fontName, textWidth, textHeight);

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba(cr, color.red, color.blue, color.green, color.alpha);
  cairo_set_font_options(cr, gdk_screen_get_font_options(screen));
  layout = pango_cairo_create_layout(cr);
  desc = pango_font_description_from_string(fontName);
  pango_layout_set_font_description(layout, desc);
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
  pango_layout_set_markup_with_accel(layout, _text.c_str(), -1, '_', NULL);
  pangoCtx = pango_layout_get_context(layout);  // is not ref'ed
  pango_cairo_context_set_font_options(pangoCtx,
                                       gdk_screen_get_font_options(screen));
  g_object_get(settings, "gtk-xft-dpi", &dpi, NULL);
  if (dpi == -1)
  {
    // use some default DPI-value
    pango_cairo_context_set_resolution(pangoCtx, 96.0f);
  }
  else
  {
    pango_cairo_context_set_resolution(pangoCtx,
                                       (float) dpi / (float) PANGO_SCALE);
  }

  pango_layout_context_changed(layout);

  cairo_move_to(cr,
                2 * ITEM_MARGIN + ITEM_INDENT_ABS,
                (float)(height - textHeight) / 2.0f);
  pango_cairo_show_layout(cr, layout);

  // clean up
  pango_font_description_free(desc);
  g_free(fontName);
  g_object_unref(layout);
  cairo_destroy(cr);
}

void QuicklistMenuItem::DrawPrelight(nux::CairoGraphics* cairo, int width, int height, nux::Color const& color)
{
  if (!cairo)
    return;

  cairo_t* cr = cairo->GetContext();

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba(cr, color.red, color.blue, color.green, color.alpha);
  cairo->DrawRoundedRectangle(cr, 1.0f, 0.0f, 0.0f, ITEM_CORNER_RADIUS_ABS,
                              width, height);
  cairo_fill(cr);
  cairo_destroy(cr);
}

void
QuicklistMenuItem::EnableLabelMarkup(bool enabled)
{
  if (IsMarkupEnabled() != enabled)
  {
    dbusmenu_menuitem_property_set_bool(_menuItem, "unity-use-markup", enabled);

    _text = "";
    InitializeText();
  }
}

bool
QuicklistMenuItem::IsMarkupEnabled()
{
  gboolean markup;

  if (!_menuItem)
    return false;

  markup = dbusmenu_menuitem_property_get_bool(_menuItem, "unity-use-markup");
  return (markup != FALSE);
}

// Introspection

std::string QuicklistMenuItem::GetName() const
{
  return _name;
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
  .add("selected", _prelight);
}

} //NAMESPACE
