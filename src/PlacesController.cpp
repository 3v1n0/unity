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

#include "ubus-server.h"
#include "UBusMessages.h"

#include "PlacesController.h"

PlacesController::PlacesController ()
: _visible (false)
{
  // register interest with ubus so that we get activation messages
  UBusServer *ubus = ubus_server_get_default ();
  ubus_server_register_interest (ubus, UBUS_DASH_EXTERNAL_ACTIVATION,
                                 (UBusCallback)&PlacesController::ExternalActivation,
                                 this);
  ubus_server_register_interest (ubus, UBUS_PLACE_VIEW_CLOSE_REQUEST,
                                 (UBusCallback)&PlacesController::CloseRequest,
                                 this);

  _factory = new PlaceFactoryFile ();

  _window_layout = new nux::HLayout ();
  
  _window = new nux::BaseWindow ("Dash");
  _window->SetBackgroundColor (nux::Color (0.0, 0.0, 0.0, 0.85));
  _window->SinkReference ();
  _window->SetConfigureNotifyCallback(&PlacesController::WindowConfigureCallback, this);
  _window->ShowWindow(false);
  _window->InputWindowEnableStruts(false);

  _window->OnMouseDownOutsideArea.connect (sigc::mem_fun (this, &PlacesController::RecvMouseDownOutsideOfView));

  _view = new PlacesView (_factory);
  _window_layout->AddView (_view, 1);
  _window_layout->SetContentDistribution(nux::eStackLeft);
  _window_layout->SetVerticalExternalMargin(0);
  _window_layout->SetHorizontalExternalMargin(0);

  _window->SetLayout (_window_layout);
  // Set a InputArea to get the keyboard focus when window receives the enter focus event.
  _window->SetEnterFocusInputArea (_view->GetTextEntryView ());

  _view->entry_changed.connect (sigc::mem_fun (this, &PlacesController::OnActivePlaceEntryChanged));

  PlacesSettings::GetDefault ()->changed.connect (sigc::mem_fun (this, &PlacesController::OnSettingsChanged));
}

PlacesController::~PlacesController ()
{
  _window->UnReference ();
}

void PlacesController::Show ()
{
  if (_visible)
    return;

  _window->ShowWindow (true, false);
  // Raise this window on top of all other BaseWindows
  _window->PushToFront ();
  _window->EnableInputWindow (true, "places", false, true);
  _window->GrabPointer ();
  _window->GrabKeyboard ();
  _window->QueueDraw ();
  _window->CaptureMouseDownAnyWhereElse (true);

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
  _window->ShowWindow (false, false);
 
  _visible = false;

  _view->SetActiveEntry (NULL, 0, "");

  ubus_server_send_message (ubus_server_get_default (),  UBUS_PLACE_VIEW_HIDDEN, NULL);
}

void PlacesController::ToggleShowHide ()
{
  _visible ? Hide () : Show ();
}

/* Configure callback for the window */
void
PlacesController::WindowConfigureCallback(int WindowWidth, int WindowHeight, nux::Geometry& geo, void *user_data)
{
  PlacesSettings *settings = PlacesSettings::GetDefault ();
  GdkScreen      *screen;
  gint            primary_monitor, width=0, height=0;
  GdkRectangle    rect;
  gint            tile_width;

  screen = gdk_screen_get_default ();
  primary_monitor = gdk_screen_get_primary_monitor (screen);
  gdk_screen_get_monitor_geometry (screen, primary_monitor, &rect);

  tile_width = settings->GetDefaultTileWidth (); 

  if (settings->GetFormFactor () == PlacesSettings::DESKTOP)
  {
    gint half = rect.width / 2;

    while ((width + tile_width) <= half)
      width += tile_width;
    
    width = MAX (width, tile_width * 7);
    height = ((width/tile_width) - 3) * tile_width;
  }
  else
  {
    width = rect.width - 66;
    height = rect.height - 24;
  }

  geo = nux::Geometry (66, 24, width, height);
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
  //FIXME: Lots of things to detect here still
  Hide ();
}

void
PlacesController::OnActivePlaceEntryChanged (PlaceEntry *entry)
{
  entry ? Show () : Hide ();
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
