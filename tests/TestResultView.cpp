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
#include "Nux/Button.h"
#include "Nux/TextureArea.h"
#include "Nux/WindowThread.h"
#include "NuxGraphics/GraphicsEngine.h"
#include <Nux/ScrollView.h>
#include <gtk/gtk.h>

#include "ResultRendererTile.h"
#include "ResultViewGrid.h"
#include <UnityCore/Result.h>

class TestRunner
{
public:
  TestRunner ();
  ~TestRunner ();

  static void InitWindowThread (nux::NThread* thread, void* InitData);
  void Init ();
  nux::Layout *layout;

  DeeModel *model_;
  DeeModelIter* iter0_;
  DeeModelIter* iter1_;
  DeeModelIter* iter2_;

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
  g_debug ("starting test init");
  gint64 time_start = g_get_monotonic_time ();
  model_ = dee_sequence_model_new();
  dee_model_set_schema(model_, "s", "s", "u", "s", "s", "s", "s", NULL);

  iter0_ = dee_model_append(model_,
                            "/unity/test/uri/foo1",
                            "firefox.png",
                            0,
                            "foo/bar",
                            "Firefox",
                            "A comment!",
                            "application://firefox.desktop");

  g_debug ("took %f seconds to init dee", (g_get_monotonic_time () - time_start) / 1000000.0f);
  time_start = g_get_monotonic_time();

  unity::dash::ResultViewGrid* result_view = new unity::dash::ResultViewGrid (NUX_TRACKER_LOCATION);
  unity::dash::ResultRendererTile* result_renderer = new unity::dash::ResultRendererTile (NUX_TRACKER_LOCATION);

  result_view->SetModelRenderer (result_renderer);
  unity::dash::Result* result = new unity::dash::Result(model_, iter0_, NULL);

  g_debug ("took %f seconds to init the views", (g_get_monotonic_time () - time_start) / 1000000.0f);
  time_start = g_get_monotonic_time();

  for (int i = 0; i < 2000; i++)
  {
    result_view->AddResult (*result);
  }

  g_debug ("took %f seconds to add 200 items to the view", (g_get_monotonic_time () - time_start) / 1000000.0f);
  time_start = g_get_monotonic_time();

  layout = new nux::VLayout(NUX_TRACKER_LOCATION);

  nux::ScrollView* scroller = new nux::ScrollView (NUX_TRACKER_LOCATION);
  scroller->EnableVerticalScrollBar(true);
  nux::VLayout* scroll_layout = new nux::VLayout(NUX_TRACKER_LOCATION);
  scroll_layout->AddView (result_view, 0);
  scroller->SetLayout(scroll_layout);

  layout->AddView (scroller, 1, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_FULL);
  layout->SetFocused (true);

  g_debug ("took %f seconds to layout", (g_get_monotonic_time () - time_start) / 1000000.0f);
  time_start = g_get_monotonic_time();

  nux::GetGraphicsThread()->SetLayout (layout);
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


int main(int argc, char **argv)
{
  nux::SystemThread* st = NULL;
  nux::WindowThread* wt = NULL;

  // no real tests right now, just make sure we don't get any criticals and such
  // waiting on nice perceptual diff support before we can build real tests
  // for views

  g_type_init ();
  g_thread_init (NULL);
  gtk_init (&argc, &argv);

  nux::NuxInitialize(0);


  TestRunner *test_runner = new TestRunner ();
  wt = nux::CreateGUIThread(TEXT("Unity Places Tile Test"),
                            800 , 600,
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
