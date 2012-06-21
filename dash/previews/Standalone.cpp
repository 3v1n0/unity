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
#include <gtk/gtk.h>

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/WindowThread.h"
#include "NuxGraphics/GraphicsEngine.h"
#include <NuxCore/Logger.h>

#include "unity-shared/FontSettings.h"
#include "unity-shared/UnitySettings.h"

#include "Preview.h"

#define WIDTH 1024
#define HEIGHT 768

using namespace unity::dash;

class TestRunner
{
public:
  TestRunner ();
  ~TestRunner ();

  static void InitWindowThread (nux::NThread* thread, void* InitData);
  void Init ();
  nux::Layout *layout;
  previews::Preview *view;
};

TestRunner::TestRunner ()
{
}

TestRunner::~TestRunner ()
{
}

void TestRunner::Init ()
{
  layout = new nux::VLayout(NUX_TRACKER_LOCATION);

  view = new previews::Preview(); // this just makes the baseclass, you'll want to make 
  
  view->SetMinMaxSize(WIDTH, HEIGHT);
  layout->AddView (view, 1, nux::MINOR_POSITION_CENTER);
  layout->SetMinMaxSize(WIDTH, HEIGHT);


  nux::GetWindowThread()->SetLayout (layout);
}

void TestRunner::InitWindowThread(nux::NThread* thread, void* InitData)
{
  TestRunner *self =  (TestRunner *) InitData;
  self->Init ();
}

int main(int argc, char **argv)
{
  nux::WindowThread* wt = NULL;

  gtk_init (&argc, &argv);

  nux::NuxInitialize(0);
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));
  // The instances for the pseudo-singletons.
  unity::Settings settings;

  TestRunner *test_runner = new TestRunner ();
  wt = nux::CreateGUIThread(TEXT("Unity Preview"),
                            WIDTH, HEIGHT,
                            0,
                            &TestRunner::InitWindowThread,
                            test_runner);

  wt->Run (NULL);
  delete wt;
  return 0;
}


