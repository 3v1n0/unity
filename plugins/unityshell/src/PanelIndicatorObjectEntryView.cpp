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

#include <boost/algorithm/string.hpp>

#include <glib.h>
#include <pango/pangocairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <time.h>

// TODO: this include should be at the top, but it fails :(
#include "PanelIndicatorObjectEntryView.h"

#include "PanelStyle.h"
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Variant.h>


namespace unity
{

namespace
{
void draw_menu_bg(cairo_t* cr, int width, int height);
GdkPixbuf* make_pixbuf(int image_type, std::string const& image_data);
}


PanelIndicatorObjectEntryView::PanelIndicatorObjectEntryView(
  indicator::Entry::Ptr const& proxy,
  int padding)
  : TextureArea(NUX_TRACKER_LOCATION)
  , proxy_(proxy)
  , util_cg_(CAIRO_FORMAT_ARGB32, 1, 1)
  , padding_(padding)
{
  on_indicator_activate_changed_connection_ = proxy_->active_changed.connect(sigc::mem_fun(this, &PanelIndicatorObjectEntryView::OnActiveChanged));
  on_indicator_updated_connection_ = proxy_->updated.connect(sigc::mem_fun(this, &PanelIndicatorObjectEntryView::Refresh));

  on_font_changed_connection_ = g_signal_connect(gtk_settings_get_default(), "notify::gtk-font-name", (GCallback) &PanelIndicatorObjectEntryView::OnFontChanged, this);

  InputArea::OnMouseDown.connect(sigc::mem_fun(this, &PanelIndicatorObjectEntryView::OnMouseDown));
  InputArea::OnMouseUp.connect(sigc::mem_fun(this, &PanelIndicatorObjectEntryView::OnMouseUp));
  InputArea::OnMouseWheel.connect(sigc::mem_fun(this, &PanelIndicatorObjectEntryView::OnMouseWheel));

  on_panelstyle_changed_connection_ = PanelStyle::GetDefault()->changed.connect(sigc::mem_fun(this, &PanelIndicatorObjectEntryView::Refresh));
  Refresh();
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
    proxy_->set_active(true);
    //
    // ^ Set active even before the menu appears. This allows the below
    //   Refresh call to know it should draw_menu_bg() immediately
    //   rather than waiting for slow inter-process communication with
    //   unity-panel-service, which causes visible lag in many cases.
    //
  }
  Refresh();
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
  PangoLayout*          layout = NULL;
  PangoFontDescription* desc = NULL;
  PangoAttrList*        attrs = NULL;
  GtkSettings*          settings = gtk_settings_get_default();
  cairo_t*              cr;
  char*                 font_description = NULL;
  GdkScreen*            screen = gdk_screen_get_default();
  int                   dpi = 0;

  std::string label = proxy_->label();
  glib::Object<GdkPixbuf> pixbuf(make_pixbuf(proxy_->image_type(),
                                             proxy_->image_data()));


  int  x = 0;
  int  width = 0;
  int  height = PANEL_HEIGHT;
  int  icon_width = 0;
  int  text_width = 0;
  int  text_height = 0;

  if (proxy_->show_now())
  {
    if (!pango_parse_markup(label.c_str(),
                            -1,
                            '_',
                            &attrs,
                            NULL,
                            NULL,
                            NULL))
    {
      g_debug("pango_parse_markup failed");
    }
  }
  boost::erase_all(label, "_");

  // First lets figure out our size
  if (pixbuf && proxy_->image_visible())
  {
    width = gdk_pixbuf_get_width(pixbuf);
    icon_width = width;
  }

  if (!label.empty() && proxy_->label_visible())
  {
    PangoContext* cxt;
    PangoRectangle log_rect;

    cr = util_cg_.GetContext();

    g_object_get(settings,
                 "gtk-font-name", &font_description,
                 "gtk-xft-dpi", &dpi,
                 NULL);
    desc = pango_font_description_from_string(font_description);
    pango_font_description_set_weight(desc, PANGO_WEIGHT_NORMAL);

    layout = pango_cairo_create_layout(cr);
    if (attrs)
    {
      pango_layout_set_attributes(layout, attrs);
      pango_attr_list_unref(attrs);
    }

    pango_layout_set_font_description(layout, desc);
    pango_layout_set_text(layout, label.c_str(), -1);

    cxt = pango_layout_get_context(layout);
    pango_cairo_context_set_font_options(cxt, gdk_screen_get_font_options(screen));
    pango_cairo_context_set_resolution(cxt, (float)dpi / (float)PANGO_SCALE);
    pango_layout_context_changed(layout);

    pango_layout_get_extents(layout, NULL, &log_rect);
    text_width = log_rect.width / PANGO_SCALE;
    text_height = log_rect.height / PANGO_SCALE;

    if (icon_width)
      width += SPACING;
    width += text_width;

    pango_font_description_free(desc);
    g_free(font_description);
    cairo_destroy(cr);
  }

  if (width)
    width += padding_ * 2;

  SetMinimumWidth(width);

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_graphics.GetContext();
  cairo_set_line_width(cr, 1);

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  if (proxy_->active())
    draw_menu_bg(cr, width, height);

  x = padding_;

  if (pixbuf && proxy_->image_visible())
  {
    gdk_cairo_set_source_pixbuf(cr, pixbuf, x, (int)((height - gdk_pixbuf_get_height(pixbuf)) / 2));
    cairo_paint_with_alpha(cr, proxy_->image_sensitive() ? 1.0 : 0.5);

    x += icon_width + SPACING;
  }

  if (!label.empty() && proxy_->label_visible())
  {
    pango_cairo_update_layout(cr, layout);

    PanelStyle* style = PanelStyle::GetDefault();
    GtkStyleContext* style_context = style->GetStyleContext();

    gtk_style_context_save(style_context);

    GtkWidgetPath* widget_path = gtk_widget_path_new();
    gtk_widget_path_iter_set_name(widget_path, -1 , "UnityPanelWidget");
    gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_BAR);
    gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_ITEM);

    gtk_style_context_set_path(style_context, widget_path);
    gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUBAR);
    gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUITEM);

    if (proxy_->active())
      gtk_style_context_set_state(style_context, GTK_STATE_FLAG_PRELIGHT);

    gtk_render_layout(style_context, cr, x, (int)((height - text_height) / 2), layout);

    gtk_widget_path_free(widget_path);

    gtk_style_context_restore(style_context);
  }

  cairo_destroy(cr);
  if (layout)
    g_object_unref(layout);

  nux::NBitmapData* bitmap =  cairo_graphics.GetBitmap();

  // The Texture is created with a reference count of 1.
  nux::BaseTexture* texture2D = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableTexture();
  texture2D->Update(bitmap);
  delete bitmap;

  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  nux::TextureLayer* texture_layer = new nux::TextureLayer(texture2D->GetDeviceTexture(),
                                                           texxform,
                                                           nux::color::White,
                                                           true,
                                                           rop);
  SetPaintLayer(texture_layer);

  texture2D->UnReference();
  delete texture_layer;

  NeedRedraw();

  refreshed.emit(this);
}

const gchar* PanelIndicatorObjectEntryView::GetName()
{
  if (proxy_->IsUnused())
    return NULL;
  else
    return proxy_->id().c_str();
}

void PanelIndicatorObjectEntryView::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder)
  .add(GetGeometry())
  .add("label", proxy_->label())
  .add("label_sensitive", proxy_->label_sensitive())
  .add("label_visible", proxy_->label_visible())
  .add("icon_sensitive", proxy_->image_sensitive())
  .add("icon_visible", proxy_->image_visible())
  .add("active", proxy_->active());
}

bool PanelIndicatorObjectEntryView::GetShowNow()
{
  return proxy_.get() ? proxy_->show_now() : false;
}

void PanelIndicatorObjectEntryView::GetGeometryForSync(indicator::EntryLocationMap& locations)
{
  if (proxy_->IsUnused())
    return;

  locations[proxy_->id()] = GetAbsoluteGeometry();
}

bool PanelIndicatorObjectEntryView::IsEntryValid() const
{
  if (proxy_.get())
  {
    return proxy_->image_visible() || proxy_->label_visible();
  }
  return false;
}

bool PanelIndicatorObjectEntryView::IsSensitive() const
{
  if (proxy_.get())
  {
    return proxy_->image_sensitive() || proxy_->label_sensitive();
  }
  return false;
}

void PanelIndicatorObjectEntryView::OnFontChanged(GObject* gobject,
                                                  GParamSpec* pspec,
                                                  gpointer data)
{
  PanelIndicatorObjectEntryView* self = reinterpret_cast<PanelIndicatorObjectEntryView*>(data);
  self->Refresh();
}

namespace
{

void draw_menu_bg(cairo_t* cr, int width, int height)
{
  PanelStyle* style = PanelStyle::GetDefault();
  GtkStyleContext* style_context = style->GetStyleContext();

  gtk_style_context_save(style_context);

  GtkWidgetPath* widget_path = gtk_widget_path_new();
  gtk_widget_path_iter_set_name(widget_path, -1 , "UnityPanelWidget");
  gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_BAR);
  gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_ITEM);

  gtk_style_context_set_path(style_context, widget_path);
  gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUBAR);
  gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUITEM);
  gtk_style_context_set_state(style_context, GTK_STATE_FLAG_PRELIGHT);

  // FIXME(Cimi) probably some padding is needed here.
  gtk_render_background(style_context, cr, 0, 0, width, height);
  gtk_render_frame(style_context, cr, 0, 0, width, height);

  gtk_widget_path_free(widget_path);

  gtk_style_context_restore(style_context);
}

GdkPixbuf* make_pixbuf(int image_type, std::string const& image_data)
{
  GdkPixbuf* ret = NULL;

  if (image_type == GTK_IMAGE_PIXBUF)
  {
    gsize len = 0;
    guchar* decoded = g_base64_decode(image_data.c_str(), &len);

    GInputStream* stream = g_memory_input_stream_new_from_data(decoded,
                                                               len, NULL);

    ret = gdk_pixbuf_new_from_stream(stream, NULL, NULL);

    g_free(decoded);
    g_input_stream_close(stream, NULL, NULL);
    g_object_unref(stream);
  }
  else if (image_type == GTK_IMAGE_STOCK ||
           image_type == GTK_IMAGE_ICON_NAME)
  {
    ret = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                   image_data.c_str(),
                                   22,
                                   (GtkIconLookupFlags)0,
                                   NULL);
  }
  else if (image_type == GTK_IMAGE_GICON)
  {
    glib::Object<GIcon> icon(g_icon_new_for_string(image_data.c_str(), NULL));
    GtkIconInfo* info = gtk_icon_theme_lookup_by_gicon(
                          gtk_icon_theme_get_default(), icon, 22, (GtkIconLookupFlags)0);
    if (info)
    {
      ret = gtk_icon_info_load_icon(info, NULL);
      gtk_icon_info_free(info);
    }
  }

  return ret;
}

} // anon namespace


} // namespace unity
