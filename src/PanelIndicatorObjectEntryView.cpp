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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "Nux/Nux.h"
#include "Nux/HLayout.h"
#include "Nux/VLayout.h"

#include "NuxGraphics/GLThread.h"
#include "Nux/BaseWindow.h"
#include "Nux/WindowCompositor.h"

#include "PanelIndicatorObjectEntryView.h"
#include "PanelStyle.h"

#include <boost/algorithm/string.hpp>

#include <glib.h>
#include <pango/pangocairo.h>
#include <gtk/gtk.h>
#include <time.h>

namespace unity {

namespace {
void draw_menu_bg(cairo_t *cr, int width, int height);
}


PanelIndicatorObjectEntryView::PanelIndicatorObjectEntryView(
    indicator::Entry::Ptr const& proxy,
    int padding)
  : TextureArea (NUX_TRACKER_LOCATION)
  , proxy_(proxy)
  , util_cg_(CAIRO_FORMAT_ARGB32, 1, 1)
  , padding_(padding)
{
  on_indicator_activate_changed_connection_ = proxy_->active_changed.connect(sigc::mem_fun(this, &PanelIndicatorObjectEntryView::OnActiveChanged));
  on_indicator_updated_connection_ = proxy_->updated.connect(sigc::mem_fun(this, &PanelIndicatorObjectEntryView::Refresh));

  on_font_changed_connection_ = g_signal_connect (gtk_settings_get_default (), "notify::gtk-font-name", (GCallback) &PanelIndicatorObjectEntryView::OnFontChanged, this);

  InputArea::OnMouseDown.connect(sigc::mem_fun(this, &PanelIndicatorObjectEntryView::OnMouseDown));
  InputArea::OnMouseUp.connect(sigc::mem_fun (this, &PanelIndicatorObjectEntryView::OnMouseUp));
  InputArea::OnMouseWheel.connect(sigc::mem_fun(this, &PanelIndicatorObjectEntryView::OnMouseWheel));

  on_panelstyle_changed_connection_ = PanelStyle::GetDefault()->changed.connect(sigc::mem_fun(this, &PanelIndicatorObjectEntryView::Refresh));
  Refresh ();
}

PanelIndicatorObjectEntryView::~PanelIndicatorObjectEntryView()
{
  on_indicator_activate_changed_connection_.disconnect();
  on_indicator_updated_connection_.disconnect();
  on_panelstyle_changed_connection_.disconnect();
  g_signal_handler_disconnect(gtk_settings_get_default(), on_font_changed_connection_);
}

void PanelIndicatorObjectEntryView::OnActiveChanged(bool is_active)
{
  active_changed.emit(this, is_active);
}

void PanelIndicatorObjectEntryView::OnMouseDown(int x, int y,
                                                long button_flags, long key_flags)
{
  if (proxy_->active())
    return;

  if ((proxy_->label_visible() && proxy_->label_sensitive()) ||
      (proxy_->image_visible() && proxy_->image_sensitive()))
  {
    proxy_->ShowMenu(GetAbsoluteGeometry().x + 1, //cairo translation
                     GetAbsoluteGeometry().y + PANEL_HEIGHT,
                     time(NULL),
                     nux::GetEventButton(button_flags));
  } else {
	  Refresh();
  }
}

void PanelIndicatorObjectEntryView::OnMouseUp(int x, int y, long button_flags, long key_flags)
{
  Refresh();
}

void PanelIndicatorObjectEntryView::OnMouseWheel(int x, int y, int delta,
                                                 unsigned long mouse_state,
                                                 unsigned long key_state)
{
  proxy_->Scroll(delta);
}

void PanelIndicatorObjectEntryView::Activate()
{
  proxy_->ShowMenu(GetAbsoluteGeometry().x + 1, //cairo translation FIXME: Make this into one function
                   GetAbsoluteGeometry().y + PANEL_HEIGHT,
                   time(NULL),
                   1);
}

// We need to do a couple of things here:
// 1. Figure out our width
// 2. Figure out if we're active
// 3. Paint something
void PanelIndicatorObjectEntryView::Refresh()
{
  GdkPixbuf            *pixbuf = proxy_->GetPixbuf ();
  PangoLayout          *layout = NULL;
  PangoFontDescription *desc = NULL;
  PangoAttrList        *attrs = NULL;
  GtkSettings          *settings = gtk_settings_get_default ();
  cairo_t              *cr;
  char                 *font_description = NULL;
  GdkScreen            *screen = gdk_screen_get_default ();
  int                   dpi = 0;

  std::string label = proxy_->label();

  int  x = 0;
  int  y = 0;
  int  width = 0;
  int  height = PANEL_HEIGHT;
  int  icon_width = 0;
  int  text_width = 0;
  int  text_height = 0;

  PanelStyle *style = PanelStyle::GetDefault ();
  nux::Color  textcol = style->GetTextColor ();
  nux::Color  textshadowcol = style->GetTextShadow ();

  if (proxy_->show_now())
  {
    if (!pango_parse_markup (label.c_str(),
                             -1,
                             '_',
                             &attrs,
                             NULL,
                             NULL,
                             NULL))
    {
      g_debug ("pango_parse_markup failed");
    }
  }
  boost::erase_all(label, "_");

  // First lets figure out our size
  if (pixbuf && proxy_->image_visible())
  {
    width = gdk_pixbuf_get_width (pixbuf);
    icon_width = width;
  }

  if (!label.empty() && proxy_->label_visible())
  {
    PangoContext *cxt;
    PangoRectangle log_rect;

    cr = util_cg_.GetContext();

    g_object_get (settings,
                  "gtk-font-name", &font_description,
                  "gtk-xft-dpi", &dpi,
                  NULL);
    desc = pango_font_description_from_string (font_description);
    pango_font_description_set_weight (desc, PANGO_WEIGHT_NORMAL);

    layout = pango_cairo_create_layout (cr);
    if (attrs)
    {
      pango_layout_set_attributes (layout, attrs);
      pango_attr_list_unref (attrs);
    }

    pango_layout_set_font_description (layout, desc);
    pango_layout_set_text (layout, label.c_str(), -1);

    cxt = pango_layout_get_context (layout);
    pango_cairo_context_set_font_options (cxt, gdk_screen_get_font_options (screen));
    pango_cairo_context_set_resolution (cxt, (float)dpi/(float)PANGO_SCALE);
    pango_layout_context_changed (layout);

    pango_layout_get_extents (layout, NULL, &log_rect);
    text_width = log_rect.width / PANGO_SCALE;
    text_height = log_rect.height / PANGO_SCALE;

    if (icon_width)
      width += SPACING;
    width += text_width;

    pango_font_description_free (desc);
    g_free (font_description);
    cairo_destroy (cr);
  }

  if (width)
    width += padding_ *2;

  SetMinimumWidth (width);

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_graphics.GetContext();
  cairo_set_line_width (cr, 1);

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  if (proxy_->active())
    draw_menu_bg (cr, width, height);

  x = padding_;
  y = 0;

  if (pixbuf && proxy_->image_visible())
  {
    gdk_cairo_set_source_pixbuf (cr, pixbuf, x, (int)((height - gdk_pixbuf_get_height (pixbuf))/2));
    cairo_paint_with_alpha (cr, proxy_->image_sensitive() ? 1.0 : 0.5);

    x += icon_width + SPACING;
  }

  if (!label.empty() && proxy_->label_visible())
  {
    pango_cairo_update_layout (cr, layout);

    // Once for the homies that couldn't be here
    cairo_set_source_rgba (cr,
                           textshadowcol.red,
                           textshadowcol.green,
                           textshadowcol.blue,
                           1.0f - textshadowcol.red);
    cairo_move_to (cr, x, (int)(((height - text_height)/2)+1));
    pango_cairo_show_layout (cr, layout);
    cairo_stroke (cr);

    // Once again for the homies that could
    cairo_set_source_rgba (cr,
                           textcol.red,
                           textcol.green,
                           textcol.blue,
                           proxy_->label_sensitive() ? 1.0f : 0.5f);
    cairo_move_to (cr, x, (int)((height - text_height)/2));
    pango_cairo_show_layout (cr, layout);
    cairo_stroke (cr);
  }

  cairo_destroy (cr);
  if (layout)
    g_object_unref (layout);

  nux::NBitmapData* bitmap =  cairo_graphics.GetBitmap();

  // The Texture is created with a reference count of 1.
  nux::BaseTexture* texture2D = nux::GetThreadGLDeviceFactory ()->CreateSystemCapableTexture ();
  texture2D->Update(bitmap);
  delete bitmap;

  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap (nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  nux::TextureLayer* texture_layer = new nux::TextureLayer (texture2D->GetDeviceTexture(),
                                                            texxform,
                                                            nux::color::White,
                                                            true,
                                                            rop);
  SetPaintLayer (texture_layer);

  texture2D->UnReference ();
  delete texture_layer;

  NeedRedraw ();

  refreshed.emit(this);
  if (pixbuf)
    g_object_unref (pixbuf);
}

namespace {

void draw_menu_bg(cairo_t* cr, int width, int height)
{
  int radius = 4;
  double x = 0;
  double y = 0;
  double xos = 0.5;
  double yos = 0.5;
  /* FIXME */
  double mpi = 3.14159265358979323846;

  PanelStyle *style = PanelStyle::GetDefault ();
  nux::Color bgtop = style->GetBackgroundTop ();
  nux::Color bgbot = style->GetBackgroundBottom ();
  nux::Color line = style->GetLineColor ();

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  cairo_set_line_width (cr, 1.0);

  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.2);

  cairo_move_to (cr, x+xos+radius, y+yos);
  cairo_arc (cr, x+xos+width-xos*2-radius, y+yos+radius, radius, mpi*1.5, mpi*2);
  cairo_line_to (cr, x+xos+width-xos*2, y+yos+height-yos*2+2);
  cairo_line_to (cr, x+xos, y+yos+height-yos*2+2);
  cairo_arc (cr, x+xos+radius, y+yos+radius, radius, mpi, mpi*1.5);

  cairo_pattern_t * pat = cairo_pattern_create_linear (x+xos, y, x+xos, y+height-yos*2+2);
  cairo_pattern_add_color_stop_rgba (pat, 0.0,
                                     bgtop.red,
                                     bgtop.green,
                                     bgtop.blue,
                                     1.0f - bgbot.red);
  cairo_pattern_add_color_stop_rgba (pat, 1.0,
                                     bgbot.red,
                                     bgbot.green,
                                     bgbot.blue,
                                     1.0f - bgtop.red);
  cairo_set_source (cr, pat);
  cairo_fill_preserve (cr);
  cairo_pattern_destroy (pat);

  pat = cairo_pattern_create_linear (x+xos, y, x+xos, y+height-yos*2+2);
  cairo_pattern_add_color_stop_rgba (pat, 0.0,
                                     line.red,
                                     line.green,
                                     line.blue,
                                     1.0f);
  cairo_pattern_add_color_stop_rgba (pat, 1.0,
                                     line.red,
                                     line.green,
                                     line.blue,
                                     1.0f);
  cairo_set_source (cr, pat);
  cairo_stroke (cr);
  cairo_pattern_destroy (pat);

  xos++;
  yos++;

  /* enlarging the area to not draw the lightborder at bottom, ugly trick :P */
  cairo_move_to (cr, x+radius+xos, y+yos);
  cairo_arc (cr, x+xos+width-xos*2-radius, y+yos+radius, radius, mpi*1.5, mpi*2);
  cairo_line_to (cr, x+xos+width-xos*2, y+yos+height-yos*2+3);
  cairo_line_to (cr, x+xos, y+yos+height-yos*2+3);
  cairo_arc (cr, x+xos+radius, y+yos+radius, radius, mpi, mpi*1.5);

  pat = cairo_pattern_create_linear (x+xos, y, x+xos, y+height-yos*2+3);
  cairo_pattern_add_color_stop_rgba (pat, 0.0,
                                     bgbot.red,
                                     bgbot.green,
                                     bgbot.blue,
                                     1.0f);
  cairo_pattern_add_color_stop_rgba (pat, 1.0,
                                     bgbot.red,
                                     bgbot.green,
                                     bgbot.blue,
                                     1.0f);
  cairo_set_source (cr, pat);
  cairo_stroke (cr);
  cairo_pattern_destroy (pat);
}
} // anon namespace

const gchar* PanelIndicatorObjectEntryView::GetName()
{
  std::string const& name = proxy_->id();

  if (name == indicator::Entry::UNUSED_ID)
    return NULL;
  else
    return name.c_str();
}

void PanelIndicatorObjectEntryView::AddProperties (GVariantBuilder *builder)
{
  nux::Geometry geo = GetGeometry ();

  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));

  g_variant_builder_add (builder, "{sv}", "label", g_variant_new_string (proxy_->label().c_str()));
  g_variant_builder_add (builder, "{sv}", "label_sensitive", g_variant_new_boolean (proxy_->label_sensitive()));
  g_variant_builder_add (builder, "{sv}", "label_visible", g_variant_new_boolean (proxy_->label_visible()));

  g_variant_builder_add (builder, "{sv}", "icon_sensitive", g_variant_new_boolean (proxy_->image_sensitive()));
  g_variant_builder_add (builder, "{sv}", "icon_visible", g_variant_new_boolean (proxy_->image_visible()));

  g_variant_builder_add (builder, "{sv}", "active", g_variant_new_boolean (proxy_->active()));
}

bool PanelIndicatorObjectEntryView::GetShowNow()
{
  return proxy_.get() ? proxy_->show_now() : false;
}

void PanelIndicatorObjectEntryView::GetGeometryForSync(GVariantBuilder* builder,
                                                       const char* name)
{
  nux::Geometry geo = GetAbsoluteGeometry();
  g_variant_builder_add(builder, "(ssiiii)",
                        name,
                        proxy_->id().c_str(),
                        geo.x,
                        geo.y,
                        geo.GetWidth(),
                        geo.GetHeight());
}

bool PanelIndicatorObjectEntryView::IsEntryValid() const
{
  if (proxy_.get()) {
    return proxy_->image_visible() || proxy_->label_visible();
  }
  return false;
}

bool PanelIndicatorObjectEntryView::IsSensitive() const
{
  if (proxy_.get()) {
    return proxy_->image_sensitive() || proxy_->label_sensitive();
  }
  return false;
}

void PanelIndicatorObjectEntryView::OnFontChanged(GObject *gobject,
                                                  GParamSpec *pspec,
                                                  gpointer data)
{
  PanelIndicatorObjectEntryView *self = reinterpret_cast<PanelIndicatorObjectEntryView*>(data);
  self->Refresh();
}

} // namespace unity
