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
#include <gtk/gtk.h>

#include "FilterBasicButton.h"
#include "FilterRatingsWidget.h"
#include "FilterGenreWidget.h"
#include "FilterGenreButton.h"
#include "FilterMultiRangeWidget.h"
#include "PlacesStyle.h"

class TestRunner
{
public:
  TestRunner ();
  ~TestRunner ();

  static void InitWindowThread (nux::NThread* thread, void* InitData);
  void Init ();
  nux::Layout *layout;

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
  unity::FilterBasicButton *button = new unity::FilterBasicButton ("hello world", NUX_TRACKER_LOCATION);
  unity::FilterRatingsWidget *ratings = new unity::FilterRatingsWidget (NUX_TRACKER_LOCATION);
  unity::FilterGenreButton *genre_button = new unity::FilterGenreButton ("genre button", NUX_TRACKER_LOCATION);

  unity::FilterGenre *genre = new unity::FilterGenre(NUX_TRACKER_LOCATION);

  unity::FilterMultiRange *multi_range = new unity::FilterMultiRange (NUX_TRACKER_LOCATION);

  layout = new nux::VLayout(NUX_TRACKER_LOCATION);

  layout->AddView (button, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_MATCHCONTENT);
  layout->AddView (ratings, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
  layout->AddView(genre_button, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
  layout->AddView(genre, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
  layout->AddView(multi_range, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);

  layout->SetFocused (true);

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

  // The instance for the PlacesStyle.
  unity::PlacesStyle places_style;

  TestRunner *test_runner = new TestRunner ();
  wt = nux::CreateGUIThread(TEXT("Unity Places Tile Test"),
                            300 , 600,
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
