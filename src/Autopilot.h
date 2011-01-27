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
#ifndef _AUTOPILOT_H
#define _AUTOPILOT_H 1

#include <sys/time.h>

#include <glib.h>
#include <gio/gio.h>

#include <core/core.h>
#include <composite/composite.h>

#include "ubus-server.h"

#define TIMEVALDIFFU(tv1, tv2)                                              \
  (((tv1)->tv_sec == (tv2)->tv_sec || (tv1)->tv_usec >= (tv2)->tv_usec) ?   \
  ((((tv1)->tv_sec - (tv2)->tv_sec) * 1000000) +                            \
   ((tv1)->tv_usec - (tv2)->tv_usec)):                                      \
  ((((tv1)->tv_sec - 1 - (tv2)->tv_sec) * 1000000) +                        \
   (1000000 + (tv1)->tv_usec - (tv2)->tv_usec)))

#define UPDATE_TIME 5000

class AutopilotDisplay :
  public PluginClassHandler<AutopilotDisplay, CompScreen>,
  public CompositeScreenInterface
{
 public:
  AutopilotDisplay (CompScreen *screen, GDBusConnection *connection);

  void Show ();
  void StartTest (const gchar *name);

  void preparePaint (int msSinceLastPaint);
  void donePaint ();

  UBusServer *GetUBusConnection ();
  GDBusConnection *GetDBusConnection ();
 private:
  void TestTooltip ();
  void TestQuicklist ();
  void TestDragLauncher ();
  void TestDragLauncherIconAlongEdgeDrop ();
  void TestDragLauncherIconOutAndDrop ();
  void TestDragLauncherIconOutAndMove ();

  float _fps;
  float _alpha;
  float _ctime;
  float _frames;

  UBusServer *_ubus;
  GDBusConnection *_dbus; 

  CompScreen *_screen;
  CompositeScreen *_cscreen;
 
  struct timeval _last_redraw;
};

#endif /* _AUTOPILOT_H */
