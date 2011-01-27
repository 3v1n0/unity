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

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/WindowThread.h"
#include "Nux/TimeGraph.h"
#include "Nux/TimerProc.h"

#include "Autopilot.h"

nux::TimerFunctor *timer_functor;
nux::TimerHandle timer_handler;
float time_value = 0;
guint _ubus_handle;

AutopilotDisplay *AutopilotDisplay::_default = 0;

/* Static */
AutopilotDisplay*
AutopilotDisplay::GetDefault ()
{
  if (!_default)
  {
    _default = new AutopilotDisplay ();
  }

  return _default;
}

void
UpdateGraph (void *data)
{
  time_value += 0.001f;
  nux::TimeGraph *timegraph = NUX_STATIC_CAST (nux::TimeGraph*, data);
  timegraph->UpdateGraph (0, nux::GetWindowThread ()->GetFrameRate ());

  timer_handler = nux::GetTimer ().AddTimerHandler (100, timer_functor, timegraph);
}

void
InitUI (nux::NThread *thread, void *init_data)
{
  nux::VLayout *layout = new nux::VLayout (NUX_TRACKER_LOCATION);
  nux::TimeGraph *timegraph = new nux::TimeGraph (TEXT ("Graph"));
  timegraph->ShowColumnStyle ();
  timegraph->SetYAxisBounds (0.0, 200.0f);

  timegraph->AddGraph (nux::Color (0xFF9AD61F), nux::Color (0x50191919));
  timer_functor = new nux::TimerFunctor ();
  timer_functor->OnTimerExpired.connect (sigc::ptr_fun (&UpdateGraph));
  timer_handler = nux::GetTimer ().AddTimerHandler (1000, timer_functor, timegraph);

  layout->AddView (timegraph,
		   1,
		   nux::MINOR_POSITION_CENTER,
		   nux::MINOR_SIZE_FULL);
  layout->SetContentDistribution (nux::MAJOR_POSITION_CENTER);
  layout->SetHorizontalExternalMargin (4);
  layout->SetVerticalExternalMargin (4);

  nux::GetWindowThread ()->SetLayout (layout);
  nux::ColorLayer background (nux::Color (0xFF2D2D2D));
  static_cast<nux::WindowThread*> (thread)->SetWindowBackgroundPaintLayer (&background);	   
}

void
TestFinished (AutopilotDisplay *self, gchar *name, gboolean passed)
{
  GError *error = NULL;
  GVariant *result = g_variant_new ("(sb)", name, passed);

  g_dbus_connection_emit_signal (self->GetConnection (),
                                 "com.canonical.Unity.Autopilot",
                                 "/com/canonical/Unity/Debug",
                                 "com.canonical.Unity.Autopilot",
                                 "TestFinished",
                                 result,
                                 &error);
  g_variant_unref (test_name);

  if (error != NULL)
  {
    g_warning ("An error was encountered emitting TestFinished signal");
    g_error_free (error);
  }
}

void
on_tooltip_shown (GVariant payload, AutopilotDisplay *self)
{
  ubus_server_unregister_interest (_ubus, ubus_handle);
  TestFinished (self, TRUE);
}

void
AutopilotDisplay::TestToolTip ()
{
  _ubus_handle = ubus_server_register_interest (_ubus,
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


AutopilotDisplay::AutopilotDisplay ()
{
  *_ubus = ubus_server_get_default ();
}

void
AutopilotDisplay::Show ()
{
  nux::WindowThread *win = nux::CreateGUIThread (TEXT (""),
                                                 800,
                                                 600,
                                                 0,
                                                 &InitUI,
                                                 0);
  win->Run (0);
  
  delete timer_functor;
  delete win;
}

void 
AutopilotDisplay::StartTest (const gchar *name)
{
  if (g_strcmp0 (name, "show_tooltip") == 0) 
  {
    TestToolTip ();
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

void AutopilotDisplay::SetDBusConnection (GDBusConnection *connection)
{
  _connection = connection;
}
