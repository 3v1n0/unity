/*
 * Copyright 2010 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/HLayout.h"
#include "Nux/WindowThread.h"
#include "NuxGraphics/GraphicsEngine.h"
#include <gtk/gtk.h>

#include "SwitcherController.h"
#include "MockLauncherIcon.h"
#include <dbus/dbus-glib.h>

using namespace unity::switcher;
using namespace unity::ui;

static gboolean on_timeout (gpointer data)
{
  SwitcherController *self = (SwitcherController *) data;

  self->MoveNext ();

  return TRUE;
}

void ThreadWidgetInit(nux::NThread* thread, void* InitData)
{
  nux::VLayout *layout = new nux::VLayout(TEXT(""), NUX_TRACKER_LOCATION);
  SwitcherController *view = new SwitcherController ();
  view->SetWorkspace (nux::Geometry (0, 0, 1000, 500));

  //view->SetMinMaxSize(1024, 24);
  layout->SetContentDistribution(nux::eStackCenter);

  nux::GetGraphicsThread()->SetLayout (layout);

  std::vector<AbstractLauncherIcon *> icons;

  icons.push_back (new MockLauncherIcon ());
  icons.push_back (new MockLauncherIcon ());
  icons.push_back (new MockLauncherIcon ());
  icons.push_back (new MockLauncherIcon ());
  icons.push_back (new MockLauncherIcon ());
  icons.push_back (new MockLauncherIcon ());
  icons.push_back (new MockLauncherIcon ());


  view->Show (SwitcherController::ALL, SwitcherController::FOCUS_ORDER, false, icons);
  view->MoveNext ();

  g_timeout_add (1000, on_timeout, view);
}

int main(int argc, char **argv)
{
  g_type_init ();
  g_thread_init (NULL);
  gtk_init (&argc, &argv);

  dbus_g_thread_init ();

  nux::NuxInitialize(0);

  nux::WindowThread* wt = nux::CreateGUIThread(TEXT("Unity Switcher"), 1000, 500, 0, &ThreadWidgetInit, 0);
  
  wt->Run(NULL);
  delete wt;
  return 0;
}