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

#include "PreviewBasicButton.h"
#include "PreviewMusic.h"
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
  unity::dash::Preview::Properties properties;
  unity::dash::AlbumPreview* album_preview = new unity::dash::AlbumPreview (properties);

  album_preview->name = "Fron: Legacy OST";
  album_preview->artist = "Daft Funk";
  album_preview->year = "2011";
  album_preview->length = (92 * 60) + 32;
  album_preview->genres.push_back("Funk");
  album_preview->genres.push_back("Frank");

  album_preview->tracks.push_back(unity::dash::AlbumPreview::Track(0, "Underture", 90, "foo/play", "foo/pause"));
  album_preview->tracks.push_back(unity::dash::AlbumPreview::Track(1, "The Grind", 90, "foo/play", "foo/pause"));
  album_preview->tracks.push_back(unity::dash::AlbumPreview::Track(2, "Daughter of Flan", 90, "foo/play", "foo/pause"));
  album_preview->tracks.push_back(unity::dash::AlbumPreview::Track(3, "Who?", 90, "foo/play", "foo/pause"));
  album_preview->tracks.push_back(unity::dash::AlbumPreview::Track(4, "Peace", 90, "foo/play", "foo/pause"));
  album_preview->tracks.push_back(unity::dash::AlbumPreview::Track(5, "Daggerfall", 90, "foo/play", "foo/pause"));

  album_preview->album_cover = "firefox.png";
  album_preview->primary_action_name = "Play Album";
  album_preview->primary_action_icon_hint = "none";
  album_preview->primary_action_uri = "play/album";

  unity::dash::Preview::Ptr preview = unity::dash::Preview::Ptr(album_preview);
  unity::PreviewMusicAlbum* preview_view = new unity::PreviewMusicAlbum (preview, NUX_TRACKER_LOCATION);
  preview_view->UriActivated.connect ([] (std::string uri) { g_debug ("Uri Activated: %s", uri.c_str()); });
  layout = new nux::VLayout(NUX_TRACKER_LOCATION);

  layout->AddView (preview_view, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
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
