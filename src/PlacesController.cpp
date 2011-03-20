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
 */

#include "config.h"

#include "Nux/Nux.h"
#include "Nux/HLayout.h"

#include "NuxGraphics/GLThread.h"
#include <glib.h>
#include <pango/pangocairo.h>
#include <gtk/gtk.h>

#include "PlacesStyle.h"
#include "ubus-server.h"
#include "UBusMessages.h"

#include "PlacesController.h"

#define PANEL_HEIGHT 24

int PlacesController::_launcher_size = 66;

PlacesController::PlacesController ()
: _visible (false),
  _fullscren_request (false),
  _timeline_id (0)
{
  // register interest with ubus so that we get activation messages
  UBusServer *ubus = ubus_server_get_default ();
  ubus_server_register_interest (ubus, UBUS_DASH_EXTERNAL_ACTIVATION,
                                 (UBusCallback)&PlacesController::ExternalActivation,
                                 this);
  ubus_server_register_interest (ubus, UBUS_PLACE_VIEW_CLOSE_REQUEST,
                                 (UBusCallback)&PlacesController::CloseRequest,
                                 this);

  _factory = PlaceFactory::GetDefault ();

  _window_layout = new nux::HLayout ();

  _window = new nux::BaseWindow ("Dash");
  _window->SetBackgroundColor (nux::Color (0.0, 0.0, 0.0, 0.0));
  _window->SinkReference ();
  _window->SetConfigureNotifyCallback(&PlacesController::WindowConfigureCallback, this);
  _window->ShowWindow(false);
  _window->InputWindowEnableStruts(false);
  _window->SetOpacity (0.0f);

  _window->OnMouseDownOutsideArea.connect (sigc::mem_fun (this, &PlacesController::RecvMouseDownOutsideOfView));

  _view = new PlacesView (_factory);
  _window_layout->AddView (_view, 1);
  _window_layout->SetContentDistribution(nux::eStackLeft);
  _window_layout->SetVerticalExternalMargin(0);
  _window_layout->SetHorizontalExternalMargin(0);

  _window->SetLayout (_window_layout);
  // Set a InputArea to get the keyboard focus when window receives the enter focus event.
  _window->SetEnterFocusInputArea (_view->GetTextEntryView ());
  _window->SetFocused (true);

  _view->entry_changed.connect (sigc::mem_fun (this, &PlacesController::OnActivePlaceEntryChanged));
  _view->fullscreen_request.connect (sigc::mem_fun (this, &PlacesController::OnDashFullscreenRequest));

  PlacesSettings::GetDefault ()->changed.connect (sigc::mem_fun (this, &PlacesController::OnSettingsChanged));
  _view->SetFocused (true);

  Relayout (gdk_screen_get_default (), this);
  g_signal_connect (gdk_screen_get_default (), "monitors-changed",
                    G_CALLBACK (PlacesController::Relayout), this);
  g_signal_connect (gdk_screen_get_default (), "size-changed",
                    G_CALLBACK (PlacesController::Relayout), this);
}

PlacesController::~PlacesController ()
{
  _window->UnReference ();
}

void
PlacesController::Relayout (GdkScreen *screen, PlacesController *self)
{
  int width = 0, height = 0;
 
  gdk_screen_get_monitor_geometry (screen,
                                   gdk_screen_get_primary_monitor (screen),
                                   &self->_monitor_rect);

  self->GetWindowSize (&width, &height);
  self->_window->SetGeometry (nux::Geometry (self->_monitor_rect.x + _launcher_size, 
                                             self->_monitor_rect.y + PANEL_HEIGHT,
                                             width,
                                             height));
}

void PlacesController::Show ()
{
  if (_visible)
    return;

  _view->AboutToShow ();
  _window->ShowWindow (true, false);
  // Raise this window on top of all other BaseWindows
  _window->PushToFront ();
  _window->EnableInputWindow (true, "places", false, true);
  _window->GrabPointer ();
  _window->GrabKeyboard ();
  _window->QueueDraw ();
  _window->CaptureMouseDownAnyWhereElse (true);
  
  StartShowHideTimeline ();

  _visible = true;

  ubus_server_send_message (ubus_server_get_default (), UBUS_PLACE_VIEW_SHOWN, NULL);
}
void PlacesController::Hide ()
{
  if (!_visible)
    return;

  _window->CaptureMouseDownAnyWhereElse (false);
  _window->ForceStopFocus (1, 1);
  _window->UnGrabPointer ();
  _window->UnGrabKeyboard ();
  _window->EnableInputWindow (false);
  _visible = false;
  _fullscren_request = false;

  _view->SetActiveEntry (NULL, 0, "");

  StartShowHideTimeline ();
  
  ubus_server_send_message (ubus_server_get_default (),  UBUS_PLACE_VIEW_HIDDEN, NULL);
}

void PlacesController::ToggleShowHide ()
{
  _visible ? Hide () : Show ();
}

void
PlacesController::StartShowHideTimeline ()
{
  if (_timeline_id)
    g_source_remove (_timeline_id);

  _timeline_id = g_timeout_add (15, (GSourceFunc)PlacesController::OnViewShowHideFrame, this);
  _last_opacity = _window->GetOpacity ();
  _start_time = g_get_monotonic_time ();
}

gboolean
PlacesController::OnViewShowHideFrame (PlacesController *self)
{
#define _LENGTH_ 90000
  gint64 diff;
  float  progress;
  float  last_opacity;

  diff = g_get_monotonic_time () - self->_start_time;

  progress = diff/(float)_LENGTH_;
  
  last_opacity = self->_last_opacity;

  if (self->_visible)
  {
    self->_window->SetOpacity (last_opacity + ((1.0f - last_opacity) * progress));
  }
  else
  {
    self->_window->SetOpacity (last_opacity - (last_opacity * progress));
  }
 
  if (diff > _LENGTH_)
  {
    self->_timeline_id = 0;

    // Make sure the state is right
    self->_window->SetOpacity (self->_visible ? 1.0f : 0.0f);
    if (!self->_visible)
      self->_window->ShowWindow (false, false);

    return FALSE;
  }
  return TRUE;
}


void
PlacesController::GetWindowSize (int *out_width, int *out_height)
{
  PlacesSettings *settings = PlacesSettings::GetDefault ();
  PlacesStyle    *style = PlacesStyle::GetDefault ();
  GdkScreen      *screen;
  gint            primary_monitor, width=0, height=0;
  GdkRectangle    rect;
  gint            tile_width;

  screen = gdk_screen_get_default ();
  primary_monitor = gdk_screen_get_primary_monitor (screen);
  gdk_screen_get_monitor_geometry (screen, primary_monitor, &rect);

  tile_width = style->GetTileWidth ();

  if (settings->GetFormFactor () == PlacesSettings::DESKTOP && !_fullscren_request)
  {
    gint half = rect.width / 2;

    while ((width + tile_width) <= half)
      width += tile_width;

    width = MAX (width, tile_width * 7);
    height = ((width/tile_width) - 3) * tile_width;

    _view->SetSizeMode (PlacesView::SIZE_MODE_HOVER);
    style->SetDefaultNColumns (6);
  }
  else
  {
    width = rect.width - _launcher_size;
    height = rect.height - PANEL_HEIGHT;

    _view->SetSizeMode (PlacesView::SIZE_MODE_FULLSCREEN);
    style->SetDefaultNColumns (width / tile_width);
  }

  *out_width = width;
  *out_height = height;
}

/* Configure callback for the window */
void
PlacesController::WindowConfigureCallback(int WindowWidth, int WindowHeight, nux::Geometry& geo, void *user_data)
{
  PlacesController *self = static_cast<PlacesController *> (user_data);
  int width = 0, height = 0;

  self->GetWindowSize (&width, &height);
  geo = nux::Geometry (self->_monitor_rect.x + self->_launcher_size, 
                       self->_monitor_rect.y + PANEL_HEIGHT,
                       width,
                       height);
}

void
PlacesController::OnDashFullscreenRequest ()
{
  int width = 0, height = 0;
  _fullscren_request = true;
  GetWindowSize (&width, &height);
  _window->SetGeometry (nux::Geometry (_monitor_rect.x + _launcher_size, 
                                       _monitor_rect.y + PANEL_HEIGHT,
                                       width,
                                       height));
}

void
PlacesController::ExternalActivation (GVariant *data, void *val)
{
  PlacesController *self = (PlacesController*)val;
  self->ToggleShowHide ();
}

void
PlacesController::CloseRequest (GVariant *data, void *val)
{
  PlacesController *self = (PlacesController*)val;
  self->Hide ();
}

void
PlacesController::RecvMouseDownOutsideOfView  (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  nux::Geometry geo (_monitor_rect.x, _monitor_rect.y, _launcher_size, PANEL_HEIGHT);
  if (!geo.IsPointInside (x, y))
    Hide ();
}

void
PlacesController::OnActivePlaceEntryChanged (PlaceEntry *entry)
{
  entry ? Show () : Hide ();
}

void
PlacesController::SetLauncherSize (int launcher_size)
{
    _launcher_size = launcher_size;
}


/* Introspection */
const gchar *
PlacesController::GetName ()
{
  return "PlacesController";
}

void
PlacesController::AddProperties (GVariantBuilder *builder)
{
}

void
PlacesController::OnSettingsChanged (PlacesSettings *settings)
{
  // We don't need to do anything just yet over here, it's a placeholder for when we do
}
