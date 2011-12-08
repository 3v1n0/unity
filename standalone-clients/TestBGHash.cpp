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
 */
#include <Nux/Nux.h>
#include <NuxCore/Logger.h>
#include <Nux/WindowThread.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <gtk/gtk.h>

#include "BGHash.h"
#include "ubus-server.h"
#include "UBusMessages.h"

class TestRunner
{
public:
  TestRunner ();
  ~TestRunner ();

  static void InitWindowThread (nux::NThread* thread, void* InitData);
  void Init ();
  unity::BGHash bghash;
private:

};

TestRunner::TestRunner ()
{
}

TestRunner::~TestRunner ()
{
  g_debug ("test runner destroyed");
}

void TestRunner::Init ()
{
  nux::Color hash = bghash.CurrentColor ();

  g_debug ("hashed color: %f - %f - %f", hash.red, hash.green, hash.blue);

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

}

void
test_handler_color_change (GVariant *data, gpointer val)
{
  // inc a counter by two when called
  gdouble red, green, blue, alpha;
  g_variant_get (data, "(dddd)", &red, &green, &blue, &alpha);

  //g_debug ("new color: %f, %f, %f", red, green, blue);
}

int main(int argc, char **argv)
{
  UBusServer *ubus;

  nux::SystemThread* st = NULL;
  nux::WindowThread* wt = NULL;

  // no real tests right now, just make sure we don't get any criticals and such
  // waiting on nice perceptual diff support before we can build real tests
  // for views

  g_type_init ();
  g_thread_init (NULL);
  gtk_init (&argc, &argv);

  nux::NuxInitialize(0);
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));

  unity::BGHash bg_hash;

  ubus = ubus_server_get_default ();
  ubus_server_register_interest (ubus, UBUS_BACKGROUND_COLOR_CHANGED,
                                 test_handler_color_change,
                                 NULL);

  TestRunner *test_runner = new TestRunner ();
  wt = nux::CreateGUIThread(TEXT("Unity Places Tile Test"),
                            1024, 600,
                            0,
                            &TestRunner::InitWindowThread,
                            test_runner);

  st = nux::CreateSystemThread (NULL, ControlThread, wt);

  if (st)
    st->Start (NULL);

  wt->Run (NULL);
  delete st;
  delete wt;
  return 0;
}
