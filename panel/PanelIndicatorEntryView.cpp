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
#include <NuxCore/Logger.h>
#include <UnityCore/ConnectionManager.h>
#include <UnityCore/GLibWrapper.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#include "PanelIndicatorEntryView.h"

#include "unity-shared/CairoTexture.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/RawPixel.h"
#include "unity-shared/WindowManager.h"
#include "unity-shared/ThemeSettings.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UnitySettings.h"

namespace unity
{
namespace
{
DECLARE_LOGGER(logger, "unity.panel.indicator.entry");
const int DEFAULT_SPACING = 3;
const std::string IMAGE_MISSING = "image-missing";
}

using namespace indicator;

PanelIndicatorEntryView::PanelIndicatorEntryView(Entry::Ptr const& proxy, int padding,
                                                 IndicatorEntryType type)
  : TextureArea(NUX_TRACKER_LOCATION)
  , proxy_(proxy)
  , type_(type)
  , monitor_(0)
  , opacity_(1.0f)
  , draw_active_(false)
  , overlay_showing_(false)
  , disabled_(false)
  , focused_(true)
  , padding_(padding < 0 ? 0 : padding)
  , cv_(unity::Settings::Instance().em(monitor_))
{
  proxy_->active_changed.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::OnActiveChanged));
  proxy_->show_now_changed.connect(sigc::mem_fun(&show_now_changed, &sigc::signal<void, bool>::emit));
  proxy_->updated.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::Refresh));

  InputArea::mouse_down.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::OnMouseDown));
  InputArea::mouse_up.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::OnMouseUp));

  if (type_ == INDICATOR)
  {
    InputArea::SetAcceptMouseWheelEvent(true);
    InputArea::mouse_wheel.connect(sigc::mem_fun(this, &PanelIndicatorEntryView::OnMouseWheel));
  }

  auto refresh_cb = sigc::mem_fun(this, &PanelIndicatorEntryView::Refresh);
  panel::Style::Instance().changed.connect(refresh_cb);
  unity::Settings::Instance().dpi_changed.connect(refresh_cb);

  if (type_ != MENU)
    theme::Settings::Get()->icons_changed.connect(refresh_cb);

  Refresh();
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
  auto const& abs_geo = GetAbsoluteGeometry();
  proxy_->ShowMenu(abs_geo.x, abs_geo.y + abs_geo.height, button);
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
      if (overlay_showing_)
        UBusManager::SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);

      WindowManager& wm = WindowManager::Default();

      if (wm.IsExpoActive())
      {
        // Delay the activation until expo is closed
        auto conn = std::make_shared<connection::Wrapper>();
        *conn = wm.terminate_expo.connect([this, conn, button] {
          Activate(button);
          (*conn)->disconnect();
        });

        wm.TerminateExpo();
        return;
      }

      if (wm.IsScaleActive())
      {
        if (type_ == MENU)
          return;

        wm.TerminateScale();
      }

      // This is ugly... But Nux fault!
      auto const& abs_geo = GetAbsoluteGeometry();
      guint64 timestamp = nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp;
      WindowManager::Default().UnGrabMousePointer(timestamp, button, abs_geo.x, abs_geo.y);

      Activate(button);
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

glib::Object<GdkPixbuf> PanelIndicatorEntryView::MakePixbuf(int size)
{
  glib::Object<GdkPixbuf> pixbuf;

  // see if we need to do anything
  if (!proxy_->image_visible() || proxy_->image_data().empty())
    return pixbuf;

  auto image_type = proxy_->image_type();
  switch (image_type)
  {
    case GTK_IMAGE_PIXBUF:
    {
      gsize len = 0;
      auto* decoded = g_base64_decode(proxy_->image_data().c_str(), &len);
      glib::Object<GInputStream> stream(g_memory_input_stream_new_from_data(decoded, len, nullptr));
      pixbuf = gdk_pixbuf_new_from_stream(stream, nullptr, nullptr);
      g_input_stream_close(stream, nullptr, nullptr);
      g_free(decoded);
      break;
    }

    case GTK_IMAGE_ICON_NAME:
    case GTK_IMAGE_STOCK:
    case GTK_IMAGE_GICON:
    {
      GtkIconTheme* theme = gtk_icon_theme_get_default();
      auto flags = GTK_ICON_LOOKUP_FORCE_SIZE;
      glib::Object<GtkIconInfo> info;

      if (image_type == GTK_IMAGE_GICON)
      {
        glib::Object<GIcon> icon(g_icon_new_for_string(proxy_->image_data().c_str(), nullptr));
        info = gtk_icon_theme_lookup_by_gicon(theme, icon, size, flags);

        if (!info)
        {
          // Maybe the icon was just added to the theme, see if a rescan helps.
          gtk_icon_theme_rescan_if_needed(theme);
          info = gtk_icon_theme_lookup_by_gicon(theme, icon, size, flags);
        }
      }
      else
      {
        info = gtk_icon_theme_lookup_icon(theme, proxy_->image_data().c_str(), size, flags);
      }

      if (info)
      {
        auto* filename = gtk_icon_info_get_filename(info);
        pixbuf = gdk_pixbuf_new_from_file_at_size(filename, -1, size, nullptr);
        // if that failed, whine and load fallback
        if (!pixbuf)
        {
          LOG_WARN(logger) << "failed to load: " << filename;
        }
      }
      else if (image_type == GTK_IMAGE_ICON_NAME)
      {
        pixbuf = gdk_pixbuf_new_from_file_at_size(proxy_->image_data().c_str(), -1, size, nullptr);
      }

      break;
    }
  }

  // have a generic fallback pixbuf if for whatever reason icon loading
  // failed (see LP: #1525186)
  if (!pixbuf)
  {
    GtkIconTheme* theme = gtk_icon_theme_get_default();
    pixbuf = gtk_icon_theme_load_icon(theme, IMAGE_MISSING.c_str(), size, GTK_ICON_LOOKUP_FORCE_SIZE, nullptr);
  }

  return pixbuf;
}

void PanelIndicatorEntryView::DrawEntryContent(cairo_t *cr, unsigned int width, unsigned int height, glib::Object<GdkPixbuf> const& pixbuf, bool icon_scalable, glib::Object<PangoLayout> const& layout)
{
  int x = padding_;

  auto panel_item = (type_ == MENU) ? panel::PanelItem::MENU : panel::PanelItem::INDICATOR;
  GtkStyleContext* style_context = panel::Style::Instance().GetStyleContext(panel_item);
  gtk_style_context_save(style_context);

  gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUBAR);
  gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUITEM);

  if (IsActive())
  {
    gtk_style_context_set_state(style_context, GTK_STATE_FLAG_PRELIGHT);
    gtk_render_background(style_context, cr, 0, 0, width, height);
    gtk_render_frame(style_context, cr, 0, 0, width, height);
  }

  if (!IsFocused())
    gtk_style_context_set_state(style_context, GTK_STATE_FLAG_BACKDROP);

  if (pixbuf && IsIconVisible())
  {
    unsigned int icon_width = gdk_pixbuf_get_width(pixbuf);

    int y = (height - gdk_pixbuf_get_height(pixbuf)) / 2;

    if (icon_scalable)
    {
      double dpi_scale = cv_->DPIScale();
      cairo_save(cr);
      cairo_scale(cr, 1.0f/dpi_scale, 1.0f/dpi_scale);
      x = padding_ * dpi_scale;
      y = (std::ceil(height * dpi_scale) - gdk_pixbuf_get_height(pixbuf)) / 2;
      icon_width /= dpi_scale;
    }

    if (overlay_showing_ && !IsActive())
    {
      /* Most of the images we get are straight pixbufs (annoyingly), so when
       * the Overlay opens, we use the pixbuf as a mask to punch out an icon from
       * a white square. It works surprisingly well for most symbolic-type
       * icon themes/icons.
       */
      cairo_push_group(cr);
      gdk_cairo_set_source_pixbuf(cr, pixbuf, x, y);
      cairo_paint_with_alpha(cr, (IsIconSensitive() && IsFocused()) ? 1.0 : 0.5);
      std::shared_ptr<cairo_pattern_t> pat(cairo_pop_group(cr), cairo_pattern_destroy);

      cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
      cairo_rectangle(cr, x, y, width, height);
      cairo_mask(cr, pat.get());
    }
    else
    {
      cairo_push_group(cr);
      gtk_render_icon(style_context, cr, pixbuf, x, y);
      cairo_pop_group_to_source(cr);
      cairo_paint_with_alpha(cr, (IsIconSensitive() && IsFocused()) ? 1.0 : 0.5);
    }

    if (icon_scalable)
    {
      cairo_restore(cr);
      x = padding_;
    }

    x += icon_width + DEFAULT_SPACING;
  }

  if (layout)
  {
    nux::Size extents;
    pango_layout_get_pixel_size(layout, &extents.width, &extents.height);
    int y = (height - extents.height) / 2;

    if (overlay_showing_ && !IsActive())
    {
      cairo_move_to(cr, x, y);
      cairo_set_source_rgb(cr, 1.0f, 1.0f, 1.0f);
      pango_cairo_show_layout(cr, layout);
    }
    else
    {
      if (!IsActive())
      {
        // We need to render the background under the text glyphs, or the edge
        // of the text won't be correctly anti-aliased. See bug #723167
        cairo_push_group(cr);
        gtk_render_layout(style_context, cr, x, y, layout);
        std::shared_ptr<cairo_pattern_t> pat(cairo_pop_group(cr), cairo_pattern_destroy);

        cairo_push_group(cr);
        gtk_render_background(style_context, cr, 0, 0, width, height);
        cairo_pop_group_to_source(cr);
        cairo_mask(cr, pat.get());
      }

      gtk_render_layout(style_context, cr, x, y, layout);
    }
  }

  gtk_style_context_restore(style_context);
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
  auto& panel_style = panel::Style::Instance();

  double dpi_scale = cv_->DPIScale();
  int width = 0;
  int height = panel_style.PanelHeight(monitor_) / dpi_scale;
  int icon_width = 0;

  int icon_size = RawPixel((type_ != DROP_DOWN) ? 22 : 10).CP(dpi_scale);
  glib::Object<GdkPixbuf> const& pixbuf = MakePixbuf(icon_size);
  bool icon_scalable = false;

  // First lets figure out our size
  if (pixbuf && IsIconVisible())
  {
    width = gdk_pixbuf_get_width(pixbuf);

    if (gdk_pixbuf_get_height(pixbuf) == icon_size)
    {
      icon_scalable = true;
      width /= dpi_scale;
    }

    icon_width = width;
  }

  if (!label.empty() && IsLabelVisible())
  {
    PangoAttrList* attrs = nullptr;
    auto panel_item = (type_ == MENU) ? panel::PanelItem::MENU : panel::PanelItem::INDICATOR;
    std::string const& font = panel_style.GetFontDescription(panel_item);

    if (proxy_->show_now())
    {
      if (!pango_parse_markup(label.c_str(), -1, '_', &attrs, nullptr, nullptr, nullptr))
      {
        LOG_WARN(logger) << "Pango markup parsing failed";
      }
    }

    glib::Object<PangoContext> context(gdk_pango_context_get());
    std::shared_ptr<PangoFontDescription> desc(pango_font_description_from_string(font.c_str()), pango_font_description_free);
    pango_context_set_font_description(context, desc.get());
    pango_context_set_language(context, gtk_get_default_language());
    pango_cairo_context_set_resolution(context, 96.0 * Settings::Instance().font_scaling());

    label.erase(std::remove(label.begin(), label.end(), '_'), label.end());
    layout = pango_layout_new(context);
    pango_layout_set_height(layout, -1); //avoid wrap lines
    pango_layout_set_text(layout, label.c_str(), -1);
    pango_layout_set_attributes(layout, attrs);
    pango_attr_list_unref(attrs);

    nux::Size extents;
    pango_layout_get_pixel_size(layout, &extents.width, &extents.height);

    if (icon_width)
      width += DEFAULT_SPACING;

    width += extents.width;
  }

  if (width)
    width += padding_ * 2;

  SetMinMaxSize(std::ceil(width * dpi_scale), std::ceil(height * dpi_scale));
  nux::CairoGraphics cg(CAIRO_FORMAT_ARGB32, GetWidth(), GetHeight());
  cairo_surface_set_device_scale(cg.GetSurface(), dpi_scale, dpi_scale);
  cr = cg.GetInternalContext();
  cairo_set_line_width(cr, 1);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  DrawEntryContent(cr, width, height, pixbuf, icon_scalable, layout);

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
  if (monitor_ == monitor)
    return;

  monitor_ = monitor;
  cv_ = Settings::Instance().em(monitor);
  Refresh();
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
