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

#ifndef _AUTOPILOT_DISPLAY_H
#define _AUTOPILOT_DISPLAY_H 1

#include <glib.h>

#include <core/core.h>
#include <composite/composite.h>

#include "Nux/FloatingWindow.h"
#include "Nux/Nux.h"
#include "Nux/TabView.h"
#include "Nux/TimeGraph.h"
#include "Nux/TimerProc.h"
#include "Nux/VLayout.h"
#include "NuxGraphics/GraphicsEngine.h"

#include "ubus-server.h"

#define UPDATE_TIME 100

#define TIMEVALDIFFU(tv1, tv2)                                              \
  (((tv1)->tv_sec == (tv2)->tv_sec || (tv1)->tv_usec >= (tv2)->tv_usec) ?   \
  ((((tv1)->tv_sec - (tv2)->tv_sec) * 1000000) +                            \
   ((tv1)->tv_usec - (tv2)->tv_usec)):                                      \
  ((((tv1)->tv_sec - 1 - (tv2)->tv_sec) * 1000000) +                        \
   (1000000 + (tv1)->tv_usec - (tv2)->tv_usec)))

class AutopilotDisplay :
  public PluginClassHandler<AutopilotDisplay, CompScreen>,
  public CompositeScreenInterface
{
public:
  AutopilotDisplay (CompScreen *screen);
  ~AutopilotDisplay ();
  void StartTest (const gchar *name);
  void SetupTab (const gchar *name);

  CompositeScreen* GetCompositeScreen ();
  void preparePaint (int msSinceLastPaint);
  void donePaint ();

private:
  nux::TabView *_tab_view;
  nux::FloatingWindow *_window;

  CompositeScreen *_cscreen;

  float _fps;
  float _alpha;
  float _ctime;
  float _frames;

  struct timeval _last_redraw;
};


#endif /* _AUTOPILOT_DISPLAY_H */
