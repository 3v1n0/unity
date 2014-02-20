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

#include <array>

#include "PanelTray.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UnitySettings.h"

#include <NuxCore/Logger.h>

DECLARE_LOGGER(logger, "unity.panel.tray");
namespace
{
const std::string SETTINGS_NAME = "com.canonical.Unity.Panel";
const int PADDING = 3;
const std::array<std::string, 2> WHITELIST {{ "JavaEmbeddedFrame", "Wine" }};
}

namespace unity
{

PanelTray::PanelTray(int monitor)
  : View(NUX_TRACKER_LOCATION)
  , window_(gtk_window_new(GTK_WINDOW_TOPLEVEL))
  , monitor_(monitor)
{
  int panel_height = panel::Style::Instance().PanelHeight(monitor_);

  auto gtkwindow = glib::object_cast<GtkWindow>(window_);
  gtk_window_set_type_hint(gtkwindow, GDK_WINDOW_TYPE_HINT_DOCK);
  gtk_window_set_has_resize_grip(gtkwindow, FALSE);
  gtk_window_set_keep_above(gtkwindow, TRUE);
  gtk_window_set_skip_pager_hint(gtkwindow, TRUE);
  gtk_window_set_skip_taskbar_hint(gtkwindow, TRUE);
  gtk_window_resize(gtkwindow, 1, panel_height);
  gtk_window_move(gtkwindow, -panel_height,-panel_height);
  gtk_widget_set_name(window_, "UnityPanelApplet");

  gtk_widget_set_visual(window_, gdk_screen_get_rgba_visual(gdk_screen_get_default()));
  gtk_widget_realize(window_);
  gtk_widget_set_app_paintable(window_, TRUE);
  draw_signal_.Connect(window_, "draw", sigc::mem_fun(this, &PanelTray::OnTrayDraw));

  if (!g_getenv("UNITY_PANEL_TRAY_DISABLE"))
  {
    tray_ = na_tray_new_for_screen(gdk_screen_get_default(),
                                   GTK_ORIENTATION_HORIZONTAL,
                                   (NaTrayFilterCallback)FilterTrayCallback,
                                   this);
    na_tray_set_icon_size(tray_, panel_height);

    icon_removed_signal_.Connect(na_tray_get_manager(tray_), "tray_icon_removed",
                                 sigc::mem_fun(this, &PanelTray::OnTrayIconRemoved));

    gtk_container_add(GTK_CONTAINER(window_.RawPtr()), GTK_WIDGET(tray_.RawPtr()));
    gtk_widget_show(GTK_WIDGET(tray_.RawPtr()));
  }

  SetMinMaxSize(1, panel_height);
}

PanelTray::~PanelTray()
{
  if (gtk_widget_get_realized(window_))
  {
    // We call Release since we're deleting the window here manually,
    // and we don't want the smart pointer to try and delete it as well.
    gtk_widget_destroy(window_.Release());
    // We also need to release the tray to avoid the extra unref and invalid read.
    tray_.Release();
  }
}

Window PanelTray::xid()
{
  if (!window_ || !gtk_widget_get_realized(window_))
    return 0;

  return gdk_x11_window_get_xid(gtk_widget_get_window(window_));
}

void PanelTray::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  nux::Geometry const& geo = GetAbsoluteGeometry();

  gfx_context.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(gfx_context, geo);
  gfx_context.PopClippingRectangle();

  if (geo != last_geo_)
  {
    last_geo_ = geo;
    gtk_window_move(GTK_WINDOW(window_.RawPtr()), geo.x + PADDING, geo.y);
  }
}

void PanelTray::Sync()
{
  if (tray_)
  {
    SetMinMaxSize(WidthOfTray() + (PADDING * 2), panel::Style::Instance().PanelHeight(monitor_));
    QueueRelayout();
    QueueDraw();

    if (!children_.empty())
      gtk_widget_show(window_);
    else
      gtk_widget_hide(window_);
  }
}

gboolean PanelTray::FilterTrayCallback(NaTray* tray, NaTrayChild* icon, PanelTray* self)
{
  glib::String title(na_tray_child_get_title(icon));

  glib::String res_class;
  glib::String res_name;
  na_tray_child_get_wm_class(icon, &res_name, &res_class);

  bool accept = FilterTray(title.Str(), res_name.Str(), res_class.Str());

  if (accept)
  {
    if (na_tray_child_has_alpha(icon))
      na_tray_child_set_composited(icon, TRUE);

    self->children_.push_back(icon);
    self->sync_idle_.reset(new glib::Idle(sigc::mem_fun(self, &PanelTray::IdleSync)));
  }

  LOG_DEBUG(logger) << "TrayChild "
                    << (accept ? "Accepted: " : "Rejected: ")
                    << na_tray_child_get_title(icon) << " "
                    << res_name << " " << res_class;

  return accept ? TRUE : FALSE;
}

bool PanelTray::FilterTray(std::string const& title, std::string const& res_name, std::string const& res_class)
{
  for (auto const& item : WHITELIST)
    if (title.find(item) == 0 || res_name.find(item) == 0 || res_class.find(item) == 0)
      return true;

  return false;
}

void PanelTray::OnTrayIconRemoved(NaTrayManager* manager, NaTrayChild* removed)
{
  for (auto child : children_)
  {
    if (child == removed)
    {
      sync_idle_.reset(new glib::Idle(sigc::mem_fun(this, &PanelTray::IdleSync)));
      children_.remove(child);
      break;
    }
  }
}

bool PanelTray::IdleSync()
{
  int width = WidthOfTray();
  gtk_window_resize(GTK_WINDOW(window_.RawPtr()), width, panel::Style::Instance().PanelHeight(monitor_));
  Sync();

  return false;
}

int PanelTray::WidthOfTray()
{
  int width = 0;
  for (auto child: children_)
  {
    int w = gtk_widget_get_allocated_width(GTK_WIDGET(child));
    width += w > 24 ? w : 24;
  }
  return width;
}

gboolean PanelTray::OnTrayDraw(GtkWidget* widget, cairo_t* cr)
{
  GtkAllocation alloc;

  gtk_widget_get_allocation(widget, &alloc);

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.0f);
  cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
  cairo_fill(cr);

  gtk_container_propagate_draw(GTK_CONTAINER(widget),
                               gtk_bin_get_child(GTK_BIN(widget)),
                               cr);

  return FALSE;
}

std::string PanelTray::GetName() const
{
  return "Tray";
}

void PanelTray::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
  .add(GetAbsoluteGeometry())
  .add("children_count", children_.size());
}

} // namespace unity
