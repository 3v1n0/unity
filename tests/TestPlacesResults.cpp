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
  TestRunner();
  ~TestRunner();

  static void InitWindowThread(nux::NThread* thread, void* InitData);
  void Init();
  nux::VLayout* layout;
  PlacesResultsController* controller;
  PlacesResultsView* view;

private:

};

static gboolean
remove_timeout(PlacesResultsController* controller)
{
  //controller->RemoveResult ("tile1-1");

  return FALSE;
}

TestRunner::TestRunner()
{
}

TestRunner::~TestRunner()
{
}

void TestRunner::Init()
{
#if 0
  controller = new PlacesResultsController();
  view = new PlacesResultsView();

  layout = new nux::VLayout();
  layout->AddView(view, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  PlacesSimpleTile* tile11 = new PlacesSimpleTile("/usr/share/icons/scalable/apps/deluge.svg", "Der schnelle braune Fuchs sprang über den faulen Hund. Der schnelle braune Fuchs sprang über den faulen Hund. Der schnelle braune Fuchs sprang über den faulen Hund.", 50);
  PlacesSimpleTile* tile12 = new PlacesSimpleTile("/usr/share/icons/scalable/apps/deluge.svg", "Der schnelle braune Fuchs sprang über den faulen Hund. Der schnelle braune Fuchs sprang über den faulen Hund. Der schnelle braune Fuchs sprang über den faulen Hund.", 50);
  PlacesSimpleTile* tile13 = new PlacesSimpleTile("firefox", "FooBar Fox", 50);
  PlacesSimpleTile* tile14 = new PlacesSimpleTile("THISISNOTAVALIDTEXTURE.NOTREAL", "this icon is not valid", 50);

  PlacesSimpleTile* tile21 = new PlacesSimpleTile("/usr/share/icons/scalable/apps/deluge.svg", "Der schnelle braune Fuchs sprang über den faulen Hund. Der schnelle braune Fuchs sprang über den faulen Hund. Der schnelle braune Fuchs sprang über den faulen Hund.", 50);
  PlacesSimpleTile* tile22 = new PlacesSimpleTile("/usr/share/icons/scalable/apps/deluge.svg", "Der schnelle braune Fuchs sprang über den faulen Hund. Der schnelle braune Fuchs sprang über den faulen Hund. Der schnelle braune Fuchs sprang über den faulen Hund.", 50);
  PlacesSimpleTile* tile23 = new PlacesSimpleTile("firefox", "FooBar Fox", 50);
  PlacesSimpleTile* tile24 = new PlacesSimpleTile("THISISNOTAVALIDTEXTURE.NOTREAL", "this icon is not valid", 50);

  controller->SetView(view);

  controller->CreateGroup("Group1");
  controller->CreateGroup("Group3");
  controller->CreateGroup("Group2");


  controller->AddResultToGroup("Group1", tile11, tile11);
  controller->AddResultToGroup("Group1", tile12, tile12);
  controller->AddResultToGroup("Group1", tile13, tile13);
  controller->AddResultToGroup("Group1", tile14, tile14);

  controller->AddResultToGroup("Group2", tile21, tile21);
  controller->AddResultToGroup("Group2", tile22, tile22);
  controller->AddResultToGroup("Group2", tile23, tile23);
  controller->AddResultToGroup("Group2", tile24, tile24);

  for (int i = 0; i < 100; i++)
  {
    gchar name[20];
    g_snprintf((gchar*)&name, 20, "tile3-%i", i);
    controller->AddResultToGroup("Group3", new PlacesSimpleTile("firefox", "FooBar Fox", 50), &name);
  }

  for (int i = 0; i < 100; i++)
  {
    gchar name[20];
    g_snprintf((gchar*)&name, 20, "tile3-%i", i);
    controller->AddResultToGroup("Group3", new PlacesSimpleTile("firefox", "FooBar Fox", 50), name);
  }

  nux::GetGraphicsThread()->SetLayout(layout);
#endif
  g_timeout_add_seconds(2, (GSourceFunc)remove_timeout, NULL);
}

void TestRunner::InitWindowThread(nux::NThread* thread, void* InitData)
{
  TestRunner* self = (TestRunner*) InitData;
  self->Init();
}

void
ControlThread(nux::NThread* thread,
              void*         data)
{
  // sleep for 3 seconds
  nux::SleepForMilliseconds(3000);
  printf("ControlThread successfully started\n");
}


int main(int argc, char** argv)
{
  nux::SystemThread* st = NULL;
  nux::WindowThread* wt = NULL;

  // no real tests right now, just make sure we don't get any criticals and such
  // waiting on nice perceptual diff support before we can build real tests
  // for views

  g_type_init();
  
  gtk_init(&argc, &argv);

  nux::NuxInitialize(0);

  g_setenv("UNITY_ENABLE_PLACES", "1", FALSE);

  TestRunner* test_runner = new TestRunner();
  wt = nux::CreateGUIThread(TEXT("Unity Places Tile Test"),
                            1024, 600,
                            0,
                            &TestRunner::InitWindowThread,
                            test_runner);

  st = nux::CreateSystemThread(NULL, ControlThread, wt);

  if (st)
    st->Start(NULL);

  wt->Run(NULL);
  delete st;
  delete wt;
  return 0;
}
