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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/HLayout.h"
#include "Nux/TextureArea.h"
#include "Nux/WindowThread.h"
#include "NuxGraphics/GraphicsEngine.h"
#include <gtk/gtk.h>

#include "../src/ubus-server.h"

#include "PlacesView.h"
#include "PlacesController.h"
#include "UBusMessages.h"

class TestRunner
{
public:
  TestRunner ();
  ~TestRunner ();

  static void InitWindowThread (nux::NThread* thread, void* InitData);
  void Init ();
  PlacesController *controller;
  nux::HLayout *layout;

private:

};

TestRunner::TestRunner ()
{
}

TestRunner::~TestRunner ()
{
}

void TestRunner::Init ()
{
  controller = new PlacesController ();
}

void TestRunner::InitWindowThread(nux::NThread* thread, void* InitData)
{
  TestRunner *self =  (TestRunner *) InitData;
  self->Init ();
}

void
ControlThread (nux::NThread* thread,
               void*         data)
{
  // sleep for 3 seconds
  nux::SleepForMilliseconds (3000);
  printf ("ControlThread successfully started\n");

  nux::WindowThread* mainWindowThread = NUX_STATIC_CAST (nux::WindowThread*,
                                                         data);

  mainWindowThread->SetFakeEventMode (true);
  Display* display = mainWindowThread->GetWindow ().GetX11Display ();

  // assemble first button-click event
  XEvent buttonPressEvent;
  buttonPressEvent.xbutton.type        = ButtonPress;
  buttonPressEvent.xbutton.serial      = 0;
  buttonPressEvent.xbutton.send_event  = False;
  buttonPressEvent.xbutton.display     = display;
  buttonPressEvent.xbutton.window      = 0;
  buttonPressEvent.xbutton.root        = 0;
  buttonPressEvent.xbutton.subwindow   = 0;
  buttonPressEvent.xbutton.time        = CurrentTime;
  buttonPressEvent.xbutton.x           = 1000;
  buttonPressEvent.xbutton.y           = 300;
  buttonPressEvent.xbutton.x_root      = 0;
  buttonPressEvent.xbutton.y_root      = 0;
  buttonPressEvent.xbutton.state       = 0;
  buttonPressEvent.xbutton.button      = Button1;
  buttonPressEvent.xbutton.same_screen = True;

  mainWindowThread->PumpFakeEventIntoPipe (mainWindowThread,
                                           (XEvent*) &buttonPressEvent);

  while (!mainWindowThread->ReadyForNextFakeEvent ())
    nux::SleepForMilliseconds (10);

  mainWindowThread->SetFakeEventMode (false);
}


int main(int argc, char **argv)
{
  UBusServer *ubus;
  nux::SystemThread* st = NULL;
  nux::WindowThread* wt = NULL;

  g_type_init ();
  g_thread_init (NULL);
  gtk_init (&argc, &argv);

  nux::NuxInitialize(0);

  g_setenv ("UNITY_ENABLE_PLACES", "1", FALSE);

  TestRunner *test_runner = new TestRunner ();
  wt = nux::CreateGUIThread(TEXT("Unity Places"),
                            1024, 600,
                            0,
                            &TestRunner::InitWindowThread,
                            test_runner);

  st = nux::CreateSystemThread (NULL, ControlThread, wt);

  ubus = ubus_server_get_default ();
  ubus_server_send_message (ubus, UBUS_HOME_BUTTON_ACTIVATED, NULL);

  if (st)
    st->Start (NULL);

  wt->Run (NULL);
  delete st;
  delete wt;
  return 0;
}
