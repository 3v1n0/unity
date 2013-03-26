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
 *              Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */
#include <gtk/gtk.h>

#include "Nux/Nux.h"
#include "Nux/NuxTimerTickSource.h"
#include "Nux/VLayout.h"
#include "Nux/WindowThread.h"
#include "NuxGraphics/GraphicsEngine.h"
#include <NuxCore/AnimationController.h>
#include <NuxCore/Logger.h>

#include "ApplicationStarterImp.h"
#include "unity-shared/BGHash.h"
#include "unity-shared/FontSettings.h"
#include "DashView.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/ThumbnailGenerator.h"
#include "UnityCore/GSettingsScopes.h"

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
};

TestRunner::TestRunner ()
{
}

TestRunner::~TestRunner ()
{
}

void TestRunner::Init ()
{
  layout = new nux::HLayout(NUX_TRACKER_LOCATION);

  DashView* view = new DashView(std::make_shared<unity::dash::GSettingsScopes>(), 
                                std::make_shared<unity::ApplicationStarterImp>());
  view->DisableBlur();
  view->SetMinMaxSize(WIDTH, HEIGHT);
  layout->AddView (view, 1, nux::MINOR_POSITION_CENTER);
  layout->SetMinMaxSize(WIDTH, HEIGHT);

  view->AboutToShow();

  nux::GetWindowThread()->SetLayout (layout);
  nux::GetWindowCompositor().SetKeyFocusArea(view->default_focus());
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

  unity::BGHash bghash;
  unity::FontSettings font_settings;

  nux::NuxInitialize(0);
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));
  // The instances for the pseudo-singletons.
  unity::ThumbnailGenerator thumb_generator;
  unity::Settings settings;
  unity::dash::Style dash_style;
  unity::panel::Style panel_style;

  TestRunner *test_runner = new TestRunner ();
  wt = nux::CreateGUIThread(TEXT("Unity Dash"),
                            WIDTH, HEIGHT,
                            0,
                            &TestRunner::InitWindowThread,
                            test_runner);

  nux::NuxTimerTickSource tick_source;
  nux::animation::AnimationController animation_controller(tick_source);

  wt->Run (NULL);
  delete wt;
  return 0;
}
