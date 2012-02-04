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

#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>

#include <NuxGraphics/GLThread.h>
#include <Nux/BaseWindow.h>
#include <Nux/WindowCompositor.h>

#include <boost/algorithm/string.hpp>

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <time.h>

#include "CairoTexture.h"
// TODO: this include should be at the top, but it fails :(
#include "PanelIndicatorEntryView.h"

#include "PanelStyle.h"
#include <UnityCore/Variant.h>


namespace unity
{

PanelIndicatorEntryView::PanelIndicatorEntryView(
  indicator::Entry::Ptr const& proxy,
  int padding,
  IndicatorEntryType type)
  : TextureArea(NUX_TRACKER_LOCATION)
  , proxy_(proxy)
  , type_(type)
  , texture_layer_(nullptr)
  , padding_(padding < 0 ? 0 : padding)
  , opacity_(1.0f)
  , draw_active_(false)
  , dash_showing_(false)
  , disabled_(false)
{
  on_indicator_activate_changed_connection_ = proxy_->active_changed.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::OnActiveChanged));
  on_indicator_updated_connection_ = proxy_->updated.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::Refresh));

  on_font_changed_connection_ = g_signal_connect(gtk_settings_get_default(), "notify::gtk-font-name", (GCallback) &PanelIndicatorEntryView::OnFontChanged, this);

  InputArea::mouse_down.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::OnMouseDown));
  InputArea::mouse_up.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::OnMouseUp));

  InputArea::SetAcceptMouseWheelEvent(true);
  if (type_ != MENU)
    InputArea::mouse_wheel.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::OnMouseWheel));

  on_panelstyle_changed_connection_ = panel::Style::Instance().changed.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::Refresh));
  Refresh();
}

PanelIndicatorEntryView::~PanelIndicatorEntryView()
{
  on_indicator_activate_changed_connection_.disconnect();
  on_indicator_updated_connection_.disconnect();
  on_panelstyle_changed_connection_.disconnect();
  g_signal_handler_disconnect(gtk_settings_get_default(), on_font_changed_connection_);

  if (texture_layer_)
    delete texture_layer_;
}

void PanelIndicatorEntryView::OnActiveChanged(bool is_active)
{
  active_changed.emit(this, is_active);

  if (draw_active_ && !is_active)
  {
    draw_active_ = false;
    Refresh();
  }
}

void PanelIndicatorEntryView::ShowMenu(int button)
{
  proxy_->ShowMenu(0,
                   GetAbsoluteX(),
                   GetAbsoluteY() + PANEL_HEIGHT,
                   button,
                   time(nullptr));
}

void PanelIndicatorEntryView::OnMouseDown(int x, int y, long button_flags, long key_flags)
{
  if (proxy_->active() || IsDisabled())
    return;

  if (((proxy_->label_visible() && proxy_->label_sensitive()) ||
       (proxy_->image_visible() && proxy_->image_sensitive())))
  {
    int button = nux::GetEventButton(button_flags);

    if (button == 2 && type_ == INDICATOR)
      SetOpacity(0.75f);
    else
      ShowMenu(button);
  }

  Refresh();
}

void PanelIndicatorEntryView::OnMouseUp(int x, int y, long button_flags, long key_flags)
{
  if (proxy_->active() || IsDisabled())
    return;

  int button = nux::GetEventButton(button_flags);

  nux::Geometry geo = GetAbsoluteGeometry();
  int px = geo.x + x;
  int py = geo.y + y;

  if (((proxy_->label_visible() && proxy_->label_sensitive()) ||
       (proxy_->image_visible() && proxy_->image_sensitive())) &&
       button == 2 && type_ == INDICATOR)
  {
    if (geo.IsPointInside(px, py))
      proxy_->SecondaryActivate(time(nullptr));

    SetOpacity(1.0f);
  }

  Refresh();
}

void PanelIndicatorEntryView::OnMouseWheel(int x, int y, int delta,
                                           unsigned long mouse_state,
                                           unsigned long key_state)
{
  if (!IsDisabled())
    proxy_->Scroll(delta);
}

void PanelIndicatorEntryView::Activate(int button)
{
  SetActiveState(true, button);
}

void PanelIndicatorEntryView::Unactivate()
{
  SetActiveState(false, 0);
}

void PanelIndicatorEntryView::SetActiveState(bool active, int button)
{
  if (draw_active_ != active)
  {
    draw_active_ = active;
    Refresh();

    if (active)
      ShowMenu(button);
  }
}

glib::Object<GdkPixbuf> PanelIndicatorEntryView::MakePixbuf()
{
  glib::Object<GdkPixbuf> pixbuf;
  GtkIconTheme* theme = gtk_icon_theme_get_default();
  int image_type = proxy_->image_type();

  if (image_type == GTK_IMAGE_PIXBUF)
  {
    gsize len = 0;
    guchar* decoded = g_base64_decode(proxy_->image_data().c_str(), &len);

    glib::Object<GInputStream> stream(g_memory_input_stream_new_from_data(decoded,
                                                                          len,
                                                                          nullptr));

    pixbuf = gdk_pixbuf_new_from_stream(stream, nullptr, nullptr);

    g_free(decoded);
    g_input_stream_close(stream, nullptr, nullptr);
  }
  else if (image_type == GTK_IMAGE_STOCK ||
           image_type == GTK_IMAGE_ICON_NAME)
  {
    pixbuf = gtk_icon_theme_load_icon(theme, proxy_->image_data().c_str(), 22,
                                      (GtkIconLookupFlags)0, nullptr);
  }
  else if (image_type == GTK_IMAGE_GICON)
  {
    glib::Object<GIcon> icon(g_icon_new_for_string(proxy_->image_data().c_str(), nullptr));

    GtkIconInfo* info = gtk_icon_theme_lookup_by_gicon(theme, icon, 22,
                                                       (GtkIconLookupFlags)0);
    if (info)
    {
      pixbuf = gtk_icon_info_load_icon(info, nullptr);
      gtk_icon_info_free(info);
    }
  }

  return pixbuf;
}

void PanelIndicatorEntryView::DrawEntryBackground(cairo_t* cr, unsigned int width, unsigned int height)
{
  GtkStyleContext* style_context = panel::Style::Instance().GetStyleContext();

  gtk_style_context_save(style_context);

  GtkWidgetPath* widget_path = gtk_widget_path_new();
  gtk_widget_path_iter_set_name(widget_path, -1 , "UnityPanelWidget");
  gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_BAR);
  gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_ITEM);

  gtk_style_context_set_path(style_context, widget_path);
  gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUBAR);
  gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUITEM);
  gtk_style_context_set_state(style_context, GTK_STATE_FLAG_PRELIGHT);

  gtk_render_background(style_context, cr, 0, 0, width, height);
  gtk_render_frame(style_context, cr, 0, 0, width, height);

  gtk_widget_path_free(widget_path);

  gtk_style_context_restore(style_context);
}

void PanelIndicatorEntryView::DrawEntryContent(cairo_t *cr, unsigned int width, unsigned int height, glib::Object<GdkPixbuf> const& pixbuf, glib::Object<PangoLayout> const& layout)
{
  int x = padding_;

  if (IsActive())
    DrawEntryBackground(cr, width, height);

  if (pixbuf && proxy_->image_visible())
  {
    GtkStyleContext* style_context = panel::Style::Instance().GetStyleContext();
    unsigned int icon_width = gdk_pixbuf_get_width(pixbuf);

    gtk_style_context_save(style_context);

    GtkWidgetPath* widget_path = gtk_widget_path_new();
    gint pos = gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_BAR);
    pos = gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_ITEM);
    gtk_widget_path_iter_set_name(widget_path, pos, "UnityPanelWidget");

    gtk_style_context_set_path(style_context, widget_path);
    gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUBAR);
    gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUITEM);

    if (IsActive())
      gtk_style_context_set_state(style_context, GTK_STATE_FLAG_PRELIGHT);

    int y = (int)((height - gdk_pixbuf_get_height(pixbuf)) / 2);
    if (dash_showing_ && !IsActive())
    {
      /* Most of the images we get are straight pixbufs (annoyingly), so when
       * the Dash opens, we use the pixbuf as a mask to punch out an icon from
       * a white square. It works surprisingly well for most symbolic-type
       * icon themes/icons.
       */
      cairo_save(cr);

      cairo_push_group(cr);
      gdk_cairo_set_source_pixbuf(cr, pixbuf, x, y);
      cairo_paint_with_alpha(cr, proxy_->image_sensitive() ? 1.0 : 0.5);

      cairo_pattern_t* pat = cairo_pop_group(cr);

      cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
      cairo_rectangle(cr, x, y, width, height);
      cairo_mask(cr, pat);

      cairo_pattern_destroy(pat);
      cairo_restore(cr);
    }
    else
    {
      cairo_push_group(cr);
      gtk_render_icon(style_context, cr, pixbuf, x, y);
      cairo_pop_group_to_source(cr);
      cairo_paint_with_alpha(cr, proxy_->image_sensitive() ? 1.0 : 0.5);
    }

    gtk_widget_path_free(widget_path);

    gtk_style_context_restore(style_context);

    x += icon_width + SPACING;
  }

  if (layout)
  {
    PangoRectangle log_rect;
    pango_layout_get_extents(layout, nullptr, &log_rect);
    unsigned int text_height = log_rect.height / PANGO_SCALE;

    pango_cairo_update_layout(cr, layout);

    GtkStyleContext* style_context = panel::Style::Instance().GetStyleContext();

    gtk_style_context_save(style_context);

    GtkWidgetPath* widget_path = gtk_widget_path_new();
    gint pos = gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_BAR);
    pos = gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_ITEM);
    gtk_widget_path_iter_set_name(widget_path, pos, "UnityPanelWidget");

    gtk_style_context_set_path(style_context, widget_path);
    gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUBAR);
    gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUITEM);

    if (IsActive())
      gtk_style_context_set_state(style_context, GTK_STATE_FLAG_PRELIGHT);

    int y = (int)((height - text_height) / 2);
    if (dash_showing_)
    {
      cairo_move_to(cr, x, y);
      cairo_set_source_rgb(cr, 1.0f, 1.0f, 1.0f);
      pango_cairo_show_layout(cr, layout);
    }
    else
    {
      gtk_render_layout(style_context, cr, x, y, layout);
    }

    gtk_widget_path_free(widget_path);

    gtk_style_context_restore(style_context);
  }
}

// We need to do a couple of things here:
// 1. Figure out our width
// 2. Figure out if we're active
// 3. Paint something
void PanelIndicatorEntryView::Refresh()
{
  if (!IsVisible())
  {
    SetVisible(false);

    if (texture_layer_)
    {
      texture_layer_ = nullptr;
      delete texture_layer_;
    }

    QueueDraw();
    refreshed.emit(this);

    return;
  }

  glib::Object<PangoLayout> layout;
  cairo_t* cr;

  std::string label = proxy_->label();
  glib::Object<GdkPixbuf> pixbuf = MakePixbuf();

  unsigned int width = 0;
  unsigned int icon_width = 0;
  unsigned int height = PANEL_HEIGHT;
  unsigned int text_width = 0;

  // First lets figure out our size
  if (pixbuf && proxy_->image_visible())
  {
    width = gdk_pixbuf_get_width(pixbuf);
    icon_width = width;
  }

  if (!label.empty() && proxy_->label_visible())
  {
    PangoContext* cxt;
    PangoAttrList* attrs = nullptr;
    PangoRectangle log_rect;
    GdkScreen* screen = gdk_screen_get_default();
    GtkSettings* settings = gtk_settings_get_default();
    PangoFontDescription* desc = nullptr;
    glib::String font_description;
    int dpi;

    if (proxy_->show_now())
    {
      if (!pango_parse_markup(label.c_str(), -1, '_', &attrs, nullptr, nullptr, nullptr))
      {
        g_debug("pango_parse_markup failed");
      }
    }
    boost::erase_all(label, "_");

    g_object_get(settings,
                 "gtk-font-name", font_description.AsOutParam(),
                 "gtk-xft-dpi", &dpi,
                 nullptr);
    desc = pango_font_description_from_string(font_description);
    pango_font_description_set_weight(desc, PANGO_WEIGHT_NORMAL);

    nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, 1, 1);
    cr = cairo_graphics.GetContext();

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

    pango_layout_get_extents(layout, nullptr, &log_rect);
    text_width = log_rect.width / PANGO_SCALE;

    if (icon_width)
      width += SPACING;
    width += text_width;

    pango_font_description_free(desc);
    cairo_destroy(cr);
  }

  if (width)
    width += padding_ * 2;

  SetMinimumWidth(width);

  nux::CairoGraphics cg(CAIRO_FORMAT_ARGB32, width, height);
  cr = cg.GetContext();
  cairo_set_line_width(cr, 1);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);  

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  DrawEntryContent(cr, width, height, pixbuf, layout);

  nux::BaseTexture* texture2D = texture_from_cairo_graphics(cg);
  cairo_destroy(cr);

  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  if (texture_layer_)
    delete texture_layer_;

  texture_layer_ = new nux::TextureLayer(texture2D->GetDeviceTexture(), texxform,
                                         nux::color::White, true, rop);
  SetPaintLayer(texture_layer_);

  texture2D->UnReference();

  SetVisible(true);
  QueueDraw();
  refreshed.emit(this);
}

void PanelIndicatorEntryView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  if (opacity_ == 1.0f)
  {
    TextureArea::Draw(GfxContext, force_draw);
    return;
  }

  auto geo = GetGeometry();
  GfxContext.PushClippingRectangle(geo);

  if (texture_layer_)
  {
    nux::TexCoordXForm texxform;
    GfxContext.QRP_1Tex(geo.x, geo.y, geo.width, geo.height,
                        texture_layer_->GetDeviceTexture(), texxform,
                        nux::color::White * opacity_);
  }

  GfxContext.PopClippingRectangle();
}

void PanelIndicatorEntryView::DashShown()
{
  dash_showing_ = true;
  Refresh();
}

void PanelIndicatorEntryView::DashHidden()
{
  dash_showing_ = false;
  Refresh();
}

void PanelIndicatorEntryView::SetOpacity(double opacity)
{
  opacity = CLAMP(opacity, 0.0f, 1.0f);

  if (opacity_ != opacity)
  {
    opacity_ = opacity;
    NeedRedraw();
  }
}

double PanelIndicatorEntryView::GetOpacity()
{
  return opacity_;
}

std::string PanelIndicatorEntryView::GetName() const
{
  return proxy_->id().c_str();
}

void PanelIndicatorEntryView::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder)
  .add(GetGeometry())
  .add("label", proxy_->label())
  .add("label_sensitive", proxy_->label_sensitive())
  .add("label_visible", proxy_->label_visible())
  .add("icon_sensitive", proxy_->image_sensitive())
  .add("icon_visible", proxy_->image_visible())
  .add("active", proxy_->active())
  .add("priority", proxy_->priority());
}

bool PanelIndicatorEntryView::GetShowNow()
{
  return proxy_.get() ? proxy_->show_now() : false;
}

void PanelIndicatorEntryView::GetGeometryForSync(indicator::EntryLocationMap& locations)
{
  if (!IsVisible())
    return;

  locations[proxy_->id()] = GetAbsoluteGeometry();
}

bool PanelIndicatorEntryView::IsSensitive() const
{
  if (proxy_.get())
  {
    return proxy_->image_sensitive() || proxy_->label_sensitive();
  }
  return false;
}

bool PanelIndicatorEntryView::IsVisible() const
{
  if (proxy_.get())
  {
    return proxy_->visible();
  }
  return false;
}

bool PanelIndicatorEntryView::IsActive() const
{
  return draw_active_;
}

int PanelIndicatorEntryView::GetEntryPriority() const
{
  if (proxy_.get())
  {
    return proxy_->priority();
  }
  return -1;
}

void PanelIndicatorEntryView::SetDisabled(bool disabled)
{
  disabled_ = disabled;
}

bool PanelIndicatorEntryView::IsDisabled()
{
  return (disabled_ || !proxy_.get() || !IsSensitive());
}

void PanelIndicatorEntryView::OnFontChanged(GObject* gobject, GParamSpec* pspec,
                                            gpointer data)
{
  PanelIndicatorEntryView* self = reinterpret_cast<PanelIndicatorEntryView*>(data);
  self->Refresh();
}

} // namespace unity
