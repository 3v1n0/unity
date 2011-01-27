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
 * Authored by: Alex Launi <alex.launi@gmail.com>
 */

/*
#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/WindowThread.h"
#include "Nux/TimeGraph.h"
#include "Nux/TimerProc.h"
*/

#include "Autopilot.h"
#include "UBusMessages.h"

static guint tooltip_handle;

void
TestFinished (AutopilotDisplay *self, const gchar *name, gboolean passed)
{
  GError *error = NULL;
  GVariant *result = g_variant_new ("(sb)", name, passed);

  g_dbus_connection_emit_signal (self->GetDBusConnection (),
                                 "com.canonical.Unity.Autopilot",
                                 "/com/canonical/Unity/Debug",
                                 "com.canonical.Unity.Autopilot",
                                 "TestFinished",
                                 result,
                                 &error);
  g_variant_unref (result);

  if (error != NULL)
  {
    g_warning ("An error was encountered emitting TestFinished signal");
    g_error_free (error);
  }
}

void
on_tooltip_shown (GVariant *payload, AutopilotDisplay *self)
{
  ubus_server_unregister_interest (self->GetUBusConnection (), tooltip_handle);
  TestFinished (self, "show_tooltip", TRUE);
}

AutopilotDisplay::AutopilotDisplay (CompScreen *screen, GDBusConnection *connection) :
  PluginClassHandler <AutopilotDisplay, CompScreen> (screen),
  _cscreen (CompositeScreen::get (screen)),
  _fps (0),
  _ctime (0),
  _frames (0)
{
  _dbus = connection;
  _ubus = ubus_server_get_default ();
  _cscreen->preparePaintSetEnabled (this, false);
  _cscreen->donePaintSetEnabled (this, false);

}

void
AutopilotDisplay::preparePaint (int msSinceLastPaint)
{
  int timediff;
  float ratio = 0.05;
  struct timeval now;
  
  g_debug ("doing fps count");

  gettimeofday (&now, 0);
  timediff = TIMEVALDIFF (&now, &_last_redraw);

  _fps = (_fps * (1.0 - ratio)) + (1000000.0 / TIMEVALDIFFU (&now, &_last_redraw) * ratio);
  _last_redraw = now;

  _frames++;
  _ctime += timediff;

  if (1)
  {
    g_debug ("%0.0f frames in %.1f seconds = %.3f FPS",
             _frames, _ctime / 1000.0,
             _frames / (_ctime / 1000.0));

    /* reset frames and time after display */
    _frames = 0;
    _ctime = 0;
  }
  
  _cscreen->preparePaint (_alpha > 0.0 ? timediff : msSinceLastPaint);
  _alpha += timediff / 1000;
  _alpha = MIN (1.0, MAX (0.0, _alpha));
}

void
AutopilotDisplay::donePaint ()
{
  _cscreen->donePaint ();
}

GDBusConnection*
AutopilotDisplay::GetDBusConnection ()
{
  return _dbus;
}

UBusServer*
AutopilotDisplay::GetUBusConnection ()
{
  return _ubus;
}

void 
AutopilotDisplay::StartTest (const gchar *name)
{
  if (g_strcmp0 (name, "show_tooltip") == 0) 
  {
    TestTooltip ();
  }
  else if (g_strcmp0 (name, "show_quicklist") == 0)
  {
    TestQuicklist ();
  }
  else if (g_strcmp0 (name, "drag_launcher") == 0)
  {
    TestDragLauncher ();
  }
  else if (g_strcmp0 (name, "drag_launcher_icon_along_edge_drop") == 0)
  {
    TestDragLauncherIconAlongEdgeDrop ();
  }
  else if (g_strcmp0 (name, "drag_launcher_icon_out_and_drop") == 0)
  {
    TestDragLauncherIconOutAndDrop ();
  }
  else if (g_strcmp0 (name, "drag_launcher_icon_out_and_move") == 0)
  {
    TestDragLauncherIconOutAndMove ();
  }
}

void
AutopilotDisplay::TestTooltip ()
{
  tooltip_handle = ubus_server_register_interest (_ubus,
                                                  UBUS_LAUNCHER_TOOLTIP_SHOWN,
                                                  (UBusCallback) on_tooltip_shown,
                                                  this);
  // add a timeout to show test failure
}

void
AutopilotDisplay::TestQuicklist ()
{
}

void
AutopilotDisplay::TestDragLauncher ()
{
}

void
AutopilotDisplay::TestDragLauncherIconAlongEdgeDrop ()
{
}

void
AutopilotDisplay::TestDragLauncherIconOutAndDrop ()
{
}

void
AutopilotDisplay::TestDragLauncherIconOutAndMove ()
{
}

void
AutopilotDisplay::Show ()
{
  g_debug ("Beginning to count fps");
  /* enable fps counting now that we're being shown */
  _cscreen->preparePaintSetEnabled (this, true);
  _cscreen->donePaintSetEnabled (this, true);
}
