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

#include <sigc++/sigc++.h>

#include "Autopilot.h"
#include "UBusMessages.h"

struct _PrivateTestArgs
{
  Autopilot *priv;
  TestArgs *args;
};
typedef struct _PrivateTestArgs PrivateTestArgs;

static float _disp_fps;

nux::TimerFunctor *test_expiration_functor;

void
TestFinished (void *arg)
{
  GError *error = NULL;
  PrivateTestArgs *args = static_cast<PrivateTestArgs*> (arg);
  GVariant *result = g_variant_new ("(sb)", args->args->name, args->args->passed);

  g_debug ("firing TestFinished");
  g_dbus_connection_emit_signal (args->priv->GetDBusConnection (),
                                 NULL,
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

   args->priv->running_tests--;

   if (args->priv->running_tests == 0)
   {
     g_debug ("no more tests, cleaning up");
     delete test_expiration_functor;

     args->priv->GetCompositeScreen ()->preparePaintSetEnabled (args->priv, false);
     args->priv->GetCompositeScreen ()->donePaintSetEnabled (args->priv, false);
   }

   g_free (args->args->name);
   g_free (args->args);
   g_free (args);
 }

 void
 on_test_passed (GVariant *payload, PrivateTestArgs *arg)
 {
   g_debug ("test passed! woo!");
   nux::GetTimer ().RemoveTimerHandler (arg->args->expiration_handle);
   ubus_server_unregister_interest (arg->priv->GetUBusConnection (), arg->args->ubus_handle);
   arg->args->passed = TRUE;
   TestFinished (arg);
 }

 void
 RegisterUBusInterest (const gchar *signal, PrivateTestArgs *pargs)
 {
   pargs->args->ubus_handle = ubus_server_register_interest (pargs->priv->GetUBusConnection (),
                                                             signal,
                                                             (UBusCallback) on_test_passed,
                                                             pargs);

 }

 Autopilot::Autopilot (CompScreen *screen, GDBusConnection *connection) :
   PluginClassHandler <Autopilot, CompScreen> (screen),
   _cscreen (CompositeScreen::get (screen)),
   _fps (0),
   _ctime (0),
   _frames (0),
   running_tests (0)
 {
   _dbus = connection;
   _ubus = ubus_server_get_default ();
   _cscreen->preparePaintSetEnabled (this, false);
   _cscreen->donePaintSetEnabled (this, false);
 }

 void
 Autopilot::preparePaint (int msSinceLastPaint)
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
     /*g_debug ("%0.0f frames in %.1f seconds = %.3f FPS",
              _frames, _ctime / 1000.0,
              _frames / (_ctime / 1000.0));*/

     /* reset frames and time after display */
     _frames = 0;
     _ctime = 0;
   }

   _cscreen->preparePaint (_alpha > 0.0 ? timediff : msSinceLastPaint);
   _alpha += timediff / 1000;
   _alpha = MIN (1.0, MAX (0.0, _alpha));
 }

 void
 Autopilot::donePaint ()
 {
   _cscreen->donePaint ();
 }

 GDBusConnection*
 Autopilot::GetDBusConnection ()
 {
   return _dbus;
 }

 UBusServer*
 Autopilot::GetUBusConnection ()
 {
   return _ubus;
 }

 CompositeScreen*
 Autopilot::GetCompositeScreen ()
 {
   return _cscreen;
 }

 void 
 Autopilot::StartTest (const gchar *name)
 {
   std::cout << "Starting test " << name << std::endl;
   TestArgs *arg = static_cast<TestArgs*> (g_malloc (sizeof (TestArgs)));

   if (arg == NULL) {
     g_error ("Failed to allocate memory for TestArgs");
     return;
  }

  if (running_tests == 0)
  {
    test_expiration_functor = new nux::TimerFunctor ();
    test_expiration_functor->OnTimerExpired.connect (sigc::ptr_fun (&TestFinished));
  }

  running_tests++;
  
  PrivateTestArgs *pargs = static_cast<PrivateTestArgs*> (g_malloc (sizeof (PrivateTestArgs*)));
  pargs->priv = this;
  pargs->args = arg;

  arg->name = g_strdup (name);
  arg->passed = FALSE;
  arg->expiration_handle = nux::GetTimer ().AddTimerHandler (TEST_TIMEOUT, test_expiration_functor, pargs);  

  if (g_strcmp0 (name, "show_tooltip") == 0) 
  {
    RegisterUBusInterest (UBUS_TOOLTIP_SHOWN, pargs);
  }
  else if (g_strcmp0 (name, "show_quicklist") == 0)
  {
    RegisterUBusInterest (UBUS_QUICKLIST_SHOWN, pargs);
  }
  else if (g_strcmp0 (name, "drag_launcher") == 0)
  {
  }
  else if (g_strcmp0 (name, "drag_launcher_icon_along_edge_drop") == 0)
  {
  }
  else if (g_strcmp0 (name, "drag_launcher_icon_out_and_drop") == 0)
  {
  }
  else if (g_strcmp0 (name, "drag_launcher_icon_out_and_move") == 0)
  {
  }
  else
  {
    /* Some anonymous test. Will always get a failed result since we don't really know how to test it */
    return;
  }

  /* enable fps counting hooks */
  _cscreen->preparePaintSetEnabled (this, true);
  _cscreen->donePaintSetEnabled (this, true);
  CompositeScreenInterface::setHandler (_cscreen, true);
}
