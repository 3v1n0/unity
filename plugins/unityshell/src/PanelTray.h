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

#ifndef PANEL_TRAY_H
#define PANEL_TRAY_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "Introspectable.h"
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>

#include <unity-misc/na-tray.h>
#include <unity-misc/na-tray-child.h>
#include <unity-misc/na-tray-manager.h>

namespace unity
{

class PanelTray : public nux::View, public unity::debug::Introspectable
{
public:
  PanelTray();
  ~PanelTray();

  void Sync();
  Window xid();

protected:
  void Draw(nux::GraphicsEngine& gfx_content, bool force_draw);
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  static gboolean FilterTrayCallback(NaTray* tray, NaTrayChild* child, PanelTray* self);
  static gboolean IdleSync(PanelTray* tray);
  void OnTrayIconRemoved(NaTrayManager* manager, NaTrayChild* child);
  gboolean OnTrayDraw(GtkWidget* widget, cairo_t* cr);

  int WidthOfTray();

  glib::Object<GSettings> settings_;
  glib::Object<GtkWidget> window_;
  glib::Object<NaTray> tray_;
  char** whitelist_;

  glib::Signal<void, GSettings*, gchar*> whitelist_changed_;
  glib::Signal<gboolean, GtkWidget*, cairo_t*> draw_signal_;
  glib::Signal<void, NaTrayManager*, NaTrayChild*> icon_removed_signal_;
  std::list<NaTrayChild*> children_;
  nux::Geometry last_geo_;
};

}

#endif
