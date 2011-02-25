// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Alex Launi <alex.launi@canonical.com>
 */

#include <glib/gi18n-lib.h>

#include "AutopilotDisplay.h"
#include "UBusMessages.h"

static float _disp_fps;
static guint _ubus_handle;
static UBusServer *_ubus;

nux::TimerHandle _timer_handler;
nux::TimerFunctor *_timer_functor;

void
on_test_finished (GVariant *payload, AutopilotDisplay *self)
{
  ubus_server_unregister_interest (_ubus, _ubus_handle);

  self->GetCompositeScreen ()->preparePaintSetEnabled (self, false);
  self->GetCompositeScreen ()->donePaintSetEnabled (self, false);
}

void
GraphTimerInterrupt (void *data)
{
  nux::TimeGraph *timegraph = NUX_STATIC_CAST (nux::TimeGraph*, data);

  timegraph->UpdateGraph (0, _disp_fps);
  _timer_handler = nux::GetTimer ().AddTimerHandler (UPDATE_TIME, _timer_functor, timegraph);
}

AutopilotDisplay::AutopilotDisplay (CompScreen *screen) :
  PluginClassHandler <AutopilotDisplay, CompScreen> (screen),
  _cscreen (CompositeScreen::get (screen)),
  _fps (0),
  _ctime (0)
{
  _cscreen->preparePaintSetEnabled (this, false);
  _cscreen->donePaintSetEnabled (this, false);
  CompositeScreenInterface::setHandler (_cscreen, false);

  _ubus = ubus_server_get_default ();
  _tab_view = new nux::TabView (NUX_TRACKER_LOCATION);
  _window = new nux::FloatingWindow (TEXT (_("Autopilot Statistics")), NUX_TRACKER_LOCATION);
  _window->EnableInputWindow (true, 1);
  _window->AddWidget (_tab_view);

  /* set up our initial window tab for the overall stats, then tabs will be added when StartTest is called */
  SetupTab (_("Overview"));
}

AutopilotDisplay::~AutopilotDisplay ()
{
  _cscreen->preparePaintSetEnabled (this, false);
  _cscreen->donePaintSetEnabled (this, false);
  CompositeScreenInterface::setHandler (_cscreen, false);

  delete _tab_view;
  delete _window;
}

void
AutopilotDisplay::StartTest (const gchar *name)
{
  SetupTab (name);

  if (!_window->IsVisible ())
    _window->ShowWindow (true);

  _ubus_handle = ubus_server_register_interest (_ubus,
                                                UBUS_AUTOPILOT_TEST_FINISHED,
                                                (UBusCallback) on_test_finished,
                                                this);
 /* enable fps counting hooks */
  _cscreen->preparePaintSetEnabled (this, true);
  _cscreen->donePaintSetEnabled (this, true);
  CompositeScreenInterface::setHandler (_cscreen, true);
  _timer_functor = new nux::TimerFunctor ();
  _timer_functor->OnTimerExpired.connect (sigc::ptr_fun (&GraphTimerInterrupt));
}

void
AutopilotDisplay::SetupTab (const gchar *name)
{
  nux::VLayout *layout = new nux::VLayout (NUX_TRACKER_LOCATION);
  nux::TimeGraph *timegraph = new nux::TimeGraph (TEXT ("Frames per second"));
  timegraph->ShowColumnStyle ();
  timegraph->SetYAxisBounds (0.0, 200.0f);
  timegraph->AddGraph (nux::Color (0xFF9AD61F), nux::Color (0x50191919));
  _timer_handler = nux::GetTimer ().AddTimerHandler (1000, _timer_functor, timegraph);

  layout->AddView (timegraph,
                   1,
                   nux::MINOR_POSITION_CENTER,
                   nux::MINOR_SIZE_FULL);
  layout->SetContentDistribution (nux::MAJOR_POSITION_CENTER);
  layout->SetHorizontalExternalMargin (4);
  layout->SetVerticalExternalMargin (4);
 
  _tab_view->AddTab (name, layout);
}

CompositeScreen*
AutopilotDisplay::GetCompositeScreen ()
{
  return _cscreen;
}

void
AutopilotDisplay::preparePaint (int msSinceLastPaint)
{
  int timediff;
  float ratio = 0.05;
  struct timeval now;
  
  gettimeofday (&now, 0);
  timediff = TIMEVALDIFF (&now, &_last_redraw);
  
  _fps = (_fps * (1.0 - ratio)) + (1000000.0 / TIMEVALDIFFU (&now, &_last_redraw) * ratio);
  _last_redraw = now;
  
  _frames++;
  _ctime += timediff;
  
  if (1)
  {
    g_debug ("updating fps: %.3f", _frames / (_ctime / 1000.0));
    _disp_fps = _frames / (_ctime / 1000.0);
    
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
