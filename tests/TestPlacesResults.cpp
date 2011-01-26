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
#include "Nux/Button.h"
#include "IconTexture.h"
#include "StaticCairoText.h"
#include "Nux/TextureArea.h"
#include "Nux/WindowThread.h"
#include "NuxGraphics/GraphicsEngine.h"
#include "Nux/GridHLayout.h"
#include <gtk/gtk.h>

#include "PlacesSimpleTile.h"
#include "PlacesGroup.h"
#include "PlacesResultsController.h"
#include "PlacesResultsView.h"

class TestRunner
{
public:
  TestRunner ();
  ~TestRunner ();

  static void InitWindowThread (nux::NThread* thread, void* InitData);
  void Init ();
  nux::VLayout *layout;
  PlacesResultsController *controller;
  PlacesResultsView *view;

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
  controller = new PlacesResultsController ();
  view = new PlacesResultsView ();

  layout = new nux::VLayout ();
  layout->AddView (view, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  //~ PlacesGroup *group1 = new PlacesGroup ();
  //~ group1->SetTitle ("Hello World!");
  //~ group1->SetItemDetail (100, 5);
//~
  //~ nux::GridHLayout *group_content = new nux::GridHLayout (NUX_TRACKER_LOCATION);
  //~ for (int i = 0; i < 60; i++)
  //~ {
    //~ nux::ColorLayer color (nux::Color::RandomColor ());
    //~ nux::TextureArea* texture_area = new nux::TextureArea ();
    //~ texture_area->SetPaintLayer (&color);
//~
    //~ group_content->AddView (texture_area, 1, nux::eLeft, nux::eFull);
  //~ }
//~
  //~ group_content->ForceChildrenSize (true);
  //~ group_content->SetChildrenSize (64, 42);
  //~ group_content->EnablePartialVisibility (false);
//~
  //~ group_content->SetVerticalExternalMargin (4);
  //~ group_content->SetHorizontalExternalMargin (4);
  //~ group_content->SetVerticalInternalMargin (4);
  //~ group_content->SetHorizontalInternalMargin (4);
//~
  //~ group1->SetLayout (group_content);
//~
  //~ view->AddGroup (group1);
//~
//~
  //~ PlacesGroup *group2 = new PlacesGroup ();
  //~ group2->SetTitle ("Group2!");
  //~ group2->SetItemDetail (100, 5);
//~
  //~ nux::GridHLayout *group_content2 = new nux::GridHLayout (NUX_TRACKER_LOCATION);
  //~ for (int i = 0; i < 60; i++)
  //~ {
    //~ nux::ColorLayer color (nux::Color::RandomColor ());
    //~ nux::TextureArea* texture_area = new nux::TextureArea ();
    //~ texture_area->SetPaintLayer (&color);
//~
    //~ group_content2->AddView (texture_area, 1, nux::eLeft, nux::eFull);
  //~ }
//~
  //~ group_content2->ForceChildrenSize (true);
  //~ group_content2->SetChildrenSize (64, 42);
  //~ group_content2->EnablePartialVisibility (false);
//~
  //~ group_content2->SetVerticalExternalMargin (4);
  //~ group_content2->SetHorizontalExternalMargin (4);
  //~ group_content2->SetVerticalInternalMargin (4);
  //~ group_content2->SetHorizontalInternalMargin (4);
//~
  //~ group2->SetLayout (group_content2);
//~
  //~ view->AddGroup (group2);


  //~ PlacesGroup *group2 = new PlacesGroup ();
  //~ group1->SetTitle ("Second Group");
  //~ group1->SetItemDetail (100, 5);
//~
//~
//~
  PlacesSimpleTile *tile11 = new PlacesSimpleTile ("/usr/share/icons/scalable/apps/deluge.svg", "Der schnelle braune Fuchs sprang über den faulen Hund. Der schnelle braune Fuchs sprang über den faulen Hund. Der schnelle braune Fuchs sprang über den faulen Hund.", 50);
  PlacesSimpleTile *tile12 = new PlacesSimpleTile ("/usr/share/icons/scalable/apps/deluge.svg", "Der schnelle braune Fuchs sprang über den faulen Hund. Der schnelle braune Fuchs sprang über den faulen Hund. Der schnelle braune Fuchs sprang über den faulen Hund.", 50);
  PlacesSimpleTile *tile13 = new PlacesSimpleTile ("firefox", "FooBar Fox", 50);
  PlacesSimpleTile *tile14 = new PlacesSimpleTile ("THISISNOTAVALIDTEXTURE.NOTREAL", "this icon is not valid", 50);
//~ //~
  PlacesSimpleTile *tile21 = new PlacesSimpleTile ("/usr/share/icons/scalable/apps/deluge.svg", "Der schnelle braune Fuchs sprang über den faulen Hund. Der schnelle braune Fuchs sprang über den faulen Hund. Der schnelle braune Fuchs sprang über den faulen Hund.", 50);
  PlacesSimpleTile *tile22 = new PlacesSimpleTile ("/usr/share/icons/scalable/apps/deluge.svg", "Der schnelle braune Fuchs sprang über den faulen Hund. Der schnelle braune Fuchs sprang über den faulen Hund. Der schnelle braune Fuchs sprang über den faulen Hund.", 50);
  PlacesSimpleTile *tile23 = new PlacesSimpleTile ("firefox", "FooBar Fox", 50);
  PlacesSimpleTile *tile24 = new PlacesSimpleTile ("THISISNOTAVALIDTEXTURE.NOTREAL", "this icon is not valid", 50);
//~
  controller->SetView (view);
//~ //~
  controller->AddResultToGroup ("Group1", tile11, "tile1-1");
  controller->AddResultToGroup ("Group1", tile12, "tile1-2");
  controller->AddResultToGroup ("Group1", tile13, "tile1-3");
  controller->AddResultToGroup ("Group1", tile14, "tile1-4");
//~ //~
  controller->AddResultToGroup ("Group2", tile21, "tile2-1");
  controller->AddResultToGroup ("Group2", tile22, "tile2-2");
  controller->AddResultToGroup ("Group2", tile23, "tile2-3");
  controller->AddResultToGroup ("Group2", tile24, "tile2-4");

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

  nux::WindowThread* mainWindowThread = NUX_STATIC_CAST (nux::WindowThread*,
                                                         data);
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

  g_setenv ("UNITY_ENABLE_PLACES", "1", FALSE);

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
