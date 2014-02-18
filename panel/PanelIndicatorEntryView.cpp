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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <Nux/Nux.h>
#include <UnityCore/GTKWrapper.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#include "PanelIndicatorEntryView.h"

#include "unity-shared/CairoTexture.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/WindowManager.h"
#include "unity-shared/UnitySettings.h"

namespace unity
{

namespace
{
const RawPixel DEFAULT_SPACING = 3_em;

const int SCALED_IMAGE_Y = 1;
}

using namespace indicator;

PanelIndicatorEntryView::PanelIndicatorEntryView(Entry::Ptr const& proxy, int padding,
                                                 IndicatorEntryType type)
  : TextureArea(NUX_TRACKER_LOCATION)
  , proxy_(proxy)
  , spacing_(DEFAULT_SPACING)
  , left_padding_(padding < 0 ? 0 : padding)
  , right_padding_(left_padding_)
  , monitor_(0)
  , type_(type)
  , entry_texture_(nullptr)
  , opacity_(1.0f)
  , draw_active_(false)
  , overlay_showing_(false)
  , disabled_(false)
  , focused_(true)
  , cv_(unity::Settings::Instance().em(monitor_))
{
  proxy_->active_changed.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::OnActiveChanged));
  proxy_->updated.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::Refresh));

  InputArea::mouse_down.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::OnMouseDown));
  InputArea::mouse_up.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::OnMouseUp));

  if (type_ == INDICATOR)
  {
    InputArea::SetAcceptMouseWheelEvent(true);
    InputArea::mouse_wheel.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::OnMouseWheel));
  }

  panel::Style::Instance().changed.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::Refresh));

  Refresh();
}

PanelIndicatorEntryView::~PanelIndicatorEntryView()
{
  // Nothing to do...
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
  WindowManager& wm = WindowManager::Default();

  if (!wm.IsExpoActive() && !wm.IsScaleActive())
  {
    auto const& abs_geo = GetAbsoluteGeometry();
    proxy_->ShowMenu(abs_geo.x, abs_geo.y + panel::Style::Instance().PanelHeight(monitor_), button);
  }
}

void PanelIndicatorEntryView::OnMouseDown(int x, int y, long button_flags, long key_flags)
{
  if (proxy_->active() || IsDisabled())
    return;

  if (((IsLabelVisible() && IsLabelSensitive()) ||
       (IsIconVisible() && IsIconSensitive())))
  {
    int button = nux::GetEventButton(button_flags);

    if (button == 2 && type_ == INDICATOR)
    {
      SetOpacity(0.75f);
      QueueDraw();
    }
    else
    {
      ShowMenu(button);
      Refresh();
    }
  }
}

void PanelIndicatorEntryView::OnMouseUp(int x, int y, long button_flags, long key_flags)
{
  if (proxy_->active() || IsDisabled())
    return;

  int button = nux::GetEventButton(button_flags);

  nux::Geometry geo = GetAbsoluteGeometry();
  int px = geo.x + x;
  int py = geo.y + y;

  if (((IsLabelVisible() && IsLabelSensitive()) ||
       (IsIconVisible() && IsIconSensitive())) &&
      button == 2 && type_ == INDICATOR)
  {
    if (geo.IsPointInside(px, py))
      proxy_->SecondaryActivate();

    SetOpacity(1.0f);
    QueueDraw();
  }
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
  RawPixel size = (type_ != DROP_DOWN) ? 24_em : 10_em;

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
    pixbuf = gtk_icon_theme_load_icon(theme, proxy_->image_data().c_str(), size.CP(cv_),
                                      (GtkIconLookupFlags)0, nullptr);
  }
  else if (image_type == GTK_IMAGE_GICON)
  {
    glib::Object<GIcon> icon(g_icon_new_for_string(proxy_->image_data().c_str(), nullptr));

    gtk::IconInfo info(gtk_icon_theme_lookup_by_gicon(theme, icon, size.CP(cv_),
                                                      (GtkIconLookupFlags)0));
    if (info)
      pixbuf = gtk_icon_info_load_icon(info, nullptr);
  }

  return pixbuf;
}

int PanelIndicatorEntryView::PixbufWidth(glib::Object<GdkPixbuf> const& pixbuf) const
{
  int image_type = proxy_->image_type();
  if (image_type == GTK_IMAGE_PIXBUF)
  {
    return RawPixel(gdk_pixbuf_get_width(pixbuf)).CP(cv_);
  }
  else
  {
    return gdk_pixbuf_get_width(pixbuf);
  }
}

int PanelIndicatorEntryView::PixbufHeight(glib::Object<GdkPixbuf> const& pixbuf) const
{
  int image_type = proxy_->image_type();
  if (image_type == GTK_IMAGE_PIXBUF)
  {
    return RawPixel(gdk_pixbuf_get_height(pixbuf)).CP(cv_);
  }
  else
  {
    return gdk_pixbuf_get_height(pixbuf);
  }

}

void PanelIndicatorEntryView::DrawEntryPrelight(cairo_t* cr, unsigned int width, unsigned int height)
{
  GtkStyleContext* style_context = panel::Style::Instance().GetStyleContext();

  gtk_style_context_save(style_context);

  GtkWidgetPath* widget_path = gtk_widget_path_new();
  gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_BAR);
  gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_ITEM);
  gtk_widget_path_iter_set_name(widget_path, -1 , "UnityPanelWidget");

  gtk_style_context_set_path(style_context, widget_path);
  gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUBAR);
  gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUITEM);
  gtk_style_context_set_state(style_context, GTK_STATE_FLAG_PRELIGHT);

  gtk_render_background(style_context, cr, 0, 0, width, height);
  gtk_render_frame(style_context, cr, 0, 0, width, height);

  gtk_widget_path_free(widget_path);

  gtk_style_context_restore(style_context);
}

// FIXME Remove me when icons for the indicators aren't stuck as 22x22 images...
void PanelIndicatorEntryView::ScaleImageIcons(cairo_t* cr, int* x, int* y)
{
  int image_type = proxy_->image_type();
  if (image_type == GTK_IMAGE_PIXBUF)
  {
    float aspect = cv_.DPIScale();
    *x = left_padding_;
    *y = SCALED_IMAGE_Y;
    cairo_scale(cr, aspect, aspect);
  }
}

void PanelIndicatorEntryView::DrawEntryContent(cairo_t *cr, unsigned int width, unsigned int height, glib::Object<GdkPixbuf> const& pixbuf, glib::Object<PangoLayout> const& layout)
{
  int x = left_padding_.CP(cv_);

  if (IsActive())
    DrawEntryPrelight(cr, width, height);

  if (pixbuf && IsIconVisible())
  {
    GtkStyleContext* style_context = panel::Style::Instance().GetStyleContext();
    unsigned int icon_width = PixbufWidth(pixbuf);

    gtk_style_context_save(style_context);

    GtkWidgetPath* widget_path = gtk_widget_path_new();
    gint pos = gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_BAR);
    pos = gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_ITEM);
    gtk_widget_path_iter_set_name(widget_path, pos, "UnityPanelWidget");

    gtk_style_context_set_path(style_context, widget_path);
    gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUBAR);
    gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUITEM);

    if (!IsFocused())
    {
      gtk_style_context_set_state(style_context, GTK_STATE_FLAG_BACKDROP);
    }
    else if (IsActive())
    {
      gtk_style_context_set_state(style_context, GTK_STATE_FLAG_PRELIGHT);
    }

    int y = (int)((height - PixbufHeight(pixbuf)) / 2);

    if (overlay_showing_ && !IsActive())
    {
      /* Most of the images we get are straight pixbufs (annoyingly), so when
       * the Overlay opens, we use the pixbuf as a mask to punch out an icon from
       * a white square. It works surprisingly well for most symbolic-type
       * icon themes/icons.
       */
      cairo_save(cr);
      ScaleImageIcons(cr, &x, &y);

      cairo_push_group(cr);
      gdk_cairo_set_source_pixbuf(cr, pixbuf, x, y);
      cairo_paint_with_alpha(cr, (IsIconSensitive() && IsFocused()) ? 1.0 : 0.5);

      cairo_pattern_t* pat = cairo_pop_group(cr);

      cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
      cairo_rectangle(cr, x, y, width, height);
      cairo_mask(cr, pat);

      cairo_pattern_destroy(pat);
      cairo_restore(cr);
    }
    else
    {
      cairo_save(cr);
      ScaleImageIcons(cr, &x, &y);

      cairo_push_group(cr);
      gtk_render_icon(style_context, cr, pixbuf, x, y);
      cairo_pop_group_to_source(cr);
      cairo_paint_with_alpha(cr, (IsIconSensitive() && IsFocused()) ? 1.0 : 0.5);

      cairo_restore(cr);
    }

    gtk_widget_path_free(widget_path);

    gtk_style_context_restore(style_context);

    x += icon_width + spacing_.CP(cv_);
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

    if (!IsFocused())
    {
      gtk_style_context_set_state(style_context, GTK_STATE_FLAG_BACKDROP);
    }
    else if (IsActive())
    {
      gtk_style_context_set_state(style_context, GTK_STATE_FLAG_PRELIGHT);
    }

    int y = (height - text_height) / 2;

    if (overlay_showing_)
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
  if (!proxy_->visible())
  {
    SetVisible(false);
    // This will destroy the object texture. No need to manually delete the pointer
    entry_texture_ = nullptr;
    SetColor(nux::color::Transparent);

    QueueDraw();
    refreshed.emit(this);

    return;
  }

  glib::Object<PangoLayout> layout;
  cairo_t* cr;

  std::string label = GetLabel();
  glib::Object<GdkPixbuf> const& pixbuf = MakePixbuf();

  unsigned int width = 0;
  unsigned int icon_width = 0;
  unsigned int height = panel::Style::Instance().PanelHeight(monitor_);

  // First lets figure out our size
  if (pixbuf && IsIconVisible())
  {
    width = PixbufWidth(pixbuf);
    icon_width = width;
  }

  if (!label.empty() && IsLabelVisible())
  {
    using namespace panel;
    PangoContext* cxt;
    PangoAttrList* attrs = nullptr;
    PangoRectangle log_rect;
    GdkScreen* screen = gdk_screen_get_default();
    PangoFontDescription* desc = nullptr;
    PanelItem panel_item = (type_ == MENU) ? PanelItem::MENU : PanelItem::INDICATOR;

    Style& panel_style = Style::Instance();
    std::string const& font_description = panel_style.GetFontDescription(panel_item);
    int dpi = panel_style.GetTextDPI();

    if (proxy_->show_now())
    {
      if (!pango_parse_markup(label.c_str(), -1, '_', &attrs, nullptr, nullptr, nullptr))
      {
        g_debug("pango_parse_markup failed");
      }
    }

    desc = pango_font_description_from_string(font_description.c_str());
    pango_font_description_set_weight(desc, PANGO_WEIGHT_NORMAL);

    nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, 1, 1);
    cr = cairo_graphics.GetInternalContext();

    layout = pango_cairo_create_layout(cr);
    if (attrs)
    {
      pango_layout_set_attributes(layout, attrs);
      pango_attr_list_unref(attrs);
    }

    pango_layout_set_font_description(layout, desc);

    label.erase(std::remove(label.begin(), label.end(), '_'), label.end());
    pango_layout_set_text(layout, label.c_str(), -1);

    cxt = pango_layout_get_context(layout);
    pango_cairo_context_set_font_options(cxt, gdk_screen_get_font_options(screen));
    pango_cairo_context_set_resolution(cxt, dpi / static_cast<float>(PANGO_SCALE));
    pango_layout_context_changed(layout);

    pango_layout_get_extents(layout, nullptr, &log_rect);
    unsigned int text_width = log_rect.width / PANGO_SCALE;

    if (icon_width)
      width += spacing_.CP(cv_);
    width += text_width;

    pango_font_description_free(desc);
  }

  if (width)
    width += left_padding_.CP(cv_) + right_padding_.CP(cv_);

  SetMinimumWidth(width);
  SetMaximumWidth(width);

  nux::CairoGraphics cg(CAIRO_FORMAT_ARGB32, width, height);
  cr = cg.GetInternalContext();
  cairo_set_line_width(cr, 1);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  DrawEntryContent(cr, width, height, pixbuf, layout);

  entry_texture_ = texture_ptr_from_cairo_graphics(cg);
  SetTexture(entry_texture_.GetPointer());

  SetVisible(true);
  refreshed.emit(this);
  QueueDraw();
}

void PanelIndicatorEntryView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  GfxContext.PushClippingRectangle(geo);

  if (cached_geo_ != geo)
  {
    Refresh();
    cached_geo_ = geo;
  }

  if (entry_texture_ && opacity_ > 0.0f)
  {
    /* "Clear" out the background */
    nux::ROPConfig rop;
    rop.Blend = true;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

    nux::ColorLayer layer(nux::color::Transparent, true, rop);
    nux::GetPainter().PushDrawLayer(GfxContext, geo, &layer);

    nux::TexCoordXForm texxform;
    GfxContext.QRP_1Tex(geo.x, geo.y, geo.width, geo.height,
                        entry_texture_->GetDeviceTexture(), texxform,
                        nux::color::White * opacity_);
  }

  GfxContext.PopClippingRectangle();
}

void PanelIndicatorEntryView::OverlayShown()
{
  overlay_showing_ = true;
  Refresh();
}

void PanelIndicatorEntryView::OverlayHidden()
{
  overlay_showing_ = false;
  Refresh();
}

void PanelIndicatorEntryView::SetMonitor(int monitor)
{
  monitor_ = monitor;

  cv_ = unity::Settings::Instance().em(monitor);
}

void PanelIndicatorEntryView::SetOpacity(double opacity)
{
  opacity = CLAMP(opacity, 0.0f, 1.0f);

  if (opacity_ != opacity)
  {
    opacity_ = opacity;
    SetInputEventSensitivity(opacity_ != 0.0f);
    QueueDraw();
  }
}

double PanelIndicatorEntryView::GetOpacity()
{
  return opacity_;
}

PanelIndicatorEntryView::IndicatorEntryType PanelIndicatorEntryView::GetType() const
{
  return type_;
}

std::string PanelIndicatorEntryView::GetLabel() const
{
  if (proxy_.get())
  {
    return proxy_->label();
  }

  return "";
}

bool PanelIndicatorEntryView::IsLabelVisible() const
{
  if (proxy_.get())
  {
    return proxy_->label_visible();
  }

  return false;
}

bool PanelIndicatorEntryView::IsLabelSensitive() const
{
  if (proxy_.get())
  {
    return proxy_->label_sensitive();
  }

  return false;
}

bool PanelIndicatorEntryView::IsIconVisible() const
{
  if (proxy_.get())
  {
    return proxy_->image_visible();
  }

  return false;
}

bool PanelIndicatorEntryView::IsIconSensitive() const
{
  if (proxy_.get())
  {
    return proxy_->image_sensitive();
  }

  return false;
}

std::string PanelIndicatorEntryView::GetName() const
{
  return "IndicatorEntry";
}

void PanelIndicatorEntryView::AddProperties(debug::IntrospectionData& introspection)
{
  std::string type_name;

  switch (GetType())
  {
    case INDICATOR:
      type_name = "indicator";
      break;
    case MENU:
      type_name = "menu";
      break;
    case DROP_DOWN:
      type_name = "dropdown";
      break;
    default:
      type_name = "other";
  }

  introspection
  .add(GetAbsoluteGeometry())
  .add("entry_id", GetEntryID())
  .add("name_hint", proxy_->name_hint())
  .add("type", type_name)
  .add("priority", proxy_->priority())
  .add("label", GetLabel())
  .add("label_sensitive", IsLabelSensitive())
  .add("label_visible", IsLabelVisible())
  .add("icon_sensitive", IsIconSensitive())
  .add("icon_visible", IsIconVisible())
  .add("visible", IsVisible() && GetOpacity() != 0.0f)
  .add("opacity", GetOpacity())
  .add("active", proxy_->active())
  .add("menu_x", proxy_->geometry().x)
  .add("menu_y", proxy_->geometry().y)
  .add("menu_width", proxy_->geometry().width)
  .add("menu_height", proxy_->geometry().height)
  .add("menu_geo", proxy_->geometry())
  .add("focused", IsFocused());
}

bool PanelIndicatorEntryView::GetShowNow() const
{
  return proxy_.get() ? proxy_->show_now() : false;
}

void PanelIndicatorEntryView::GetGeometryForSync(EntryLocationMap& locations)
{
  if (!IsVisible())
    return;

  locations[GetEntryID()] = GetAbsoluteGeometry();
}

bool PanelIndicatorEntryView::IsSensitive() const
{
  if (proxy_.get())
  {
    return IsIconSensitive() || IsLabelSensitive();
  }
  return false;
}

bool PanelIndicatorEntryView::IsVisible()
{
  if (proxy_.get())
  {
    return TextureArea::IsVisible() && proxy_->visible();
  }

  return TextureArea::IsVisible();
}

bool PanelIndicatorEntryView::IsActive() const
{
  return draw_active_;
}

std::string PanelIndicatorEntryView::GetEntryID() const
{
  if (proxy_.get())
  {
    return proxy_->id();
  }

  return "";
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

void PanelIndicatorEntryView::SetFocusedState(bool focused)
{
  if (focused_ != focused)
  {
    focused_ = focused;
    Refresh();
  }
}

bool PanelIndicatorEntryView::IsFocused() const
{
  return focused_;
}

} // namespace unity
