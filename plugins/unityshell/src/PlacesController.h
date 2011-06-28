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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *              Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef PLACES_CONTROLLER_H
#define PLACES_CONTROLLER_H

#include <Nux/TextureArea.h>
#include <Nux/View.h>
#include "Nux/Layout.h"
#include <NuxImage/CairoGraphics.h>
#include <NuxGraphics/GraphicsEngine.h>

#include "PlaceFactoryFile.h"
#include "PlacesSettings.h"
#include "PlacesView.h"
#include "Introspectable.h"

#include <time.h>

#include <Nux/BaseWindow.h>
#include <Nux/TimelineEasings.h>

class PlacesController : public unity::Introspectable
{
public:
  PlacesController ();
  ~PlacesController ();

  void Show ();
  void Hide ();
  void ToggleShowHide ();
  static void SetLauncherSize (int launcher_size);

  nux::BaseWindow* GetWindow () {return _window;}

protected:
  const gchar* GetName ();
  void AddProperties (GVariantBuilder *builder);

private:
  static void ExternalActivation (GVariant *data, void *val);
  static void CloseRequest (GVariant *data, void *val);
  static void WindowConfigureCallback(int WindowWidth, int WindowHeight,
                                      nux::Geometry& geo, void *user_data);

  void RecvMouseDownOutsideOfView (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void OnActivePlaceEntryChanged (PlaceEntry *entry);
  void OnSettingsChanged (PlacesSettings *settings);
  void OnDashFullscreenRequest ();
  void OnCompizScreenUngrabbed ();
  void GetWindowSize (int *width, int *height);
  void StartShowHideTimeline ();
  static gboolean OnViewShowHideFrame (PlacesController *self);
  static void Relayout (GdkScreen *screen, PlacesController *self);

private:

  nux::BaseWindow  *_window;
  nux::HLayout     *_window_layout;
  PlacesView       *_view;
  PlaceFactory     *_factory;
  bool              _visible;
  bool              _fullscren_request;
  static int        _launcher_size;
  guint             _timeline_id;
  float             _last_opacity;
  gint64            _start_time;
  GdkRectangle      _monitor_rect;
  
  bool IsActivationValid ();
  struct timespec time_diff (struct timespec start, struct timespec end);
  struct timespec _last_activate_time;
  
  bool              _need_show;

  guint             _ubus_handles[2];
};

#endif // PLACES_CONTROLLER_H
