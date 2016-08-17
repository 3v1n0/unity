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
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UBusWrapper.h"
#include <UnityCore/GSettingsScopes.h>
#include <UnityCore/ScopeProxyInterface.h>

const unity::RawPixel WIDTH(1024);
const unity::RawPixel HEIGHT(768);

using namespace unity::dash;

class TestRunner
{
public:
  TestRunner(std::string const& scope, double scale)
    : scope_(scope.empty() ? "home.scope" : scope)
    , scale_(scale)
  {}

  static void InitWindowThread(nux::NThread* thread, void* InitData);
  void Init();

  std::string scope_;
  double scale_;
  nux::Layout *layout;
};

void TestRunner::Init ()
{
  layout = new nux::HLayout(NUX_TRACKER_LOCATION);

  DashView* view = new DashView(std::make_shared<unity::dash::GSettingsScopes>(),
                                std::make_shared<unity::ApplicationStarterImp>());
  view->scale = scale_;
  view->DisableBlur();
  view->SetMinMaxSize(WIDTH.CP(scale_), HEIGHT.CP(scale_));
  layout->AddView (view, 1, nux::MINOR_POSITION_CENTER);
  layout->SetMinMaxSize(WIDTH.CP(scale_), HEIGHT.CP(scale_));

  view->AboutToShow();

  nux::GetWindowThread()->SetLayout (layout);
  nux::GetWindowCompositor().SetKeyFocusArea(view->default_focus());

  unity::UBusManager::SendMessage(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                                  g_variant_new("(sus)", scope_.c_str(), GOTO_DASH_URI, ""));
}

void TestRunner::InitWindowThread(nux::NThread* thread, void* InitData)
{
  TestRunner *self =  (TestRunner *) InitData;
  self->Init();
}

int main(int argc, char **argv)
{
  gtk_init (&argc, &argv);

  unity::Settings settings;
  unity::BGHash bghash;
  unity::FontSettings font_settings;

  nux::NuxInitialize(0);
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));
  // The instances for the pseudo-singletons.
  unity::ThumbnailGenerator thumb_generator;
  unity::dash::Style dash_style;
  unity::panel::Style panel_style;

  double scale = 1.0;
  unity::glib::String scope;
  unity::glib::Error err;

  GOptionEntry args_parsed[] =
  {
    { "scope", 's', 0, G_OPTION_ARG_STRING, &scope, "The default scope ", "S" },
    { "scaling-factor", 'f', 0, G_OPTION_ARG_DOUBLE, &scale, "The dash scaling factor", "F" },
    { NULL }
  };

  std::shared_ptr<GOptionContext> ctx(g_option_context_new("Standalone Dash"), g_option_context_free);
  g_option_context_add_main_entries(ctx.get(), args_parsed, NULL);
  if (!g_option_context_parse(ctx.get(), &argc, &argv, &err))
    std::cerr << "Got error when parsing arguments: " << err << std::endl;

  TestRunner *test_runner = new TestRunner(scope.Str(), scale);
  std::unique_ptr<nux::WindowThread> wt(nux::CreateGUIThread(TEXT("Unity Dash"),
                                        WIDTH.CP(scale), HEIGHT.CP(scale),
                                        0, &TestRunner::InitWindowThread, test_runner));

  nux::ObjectPtr<nux::BaseTexture> background_tex;
  background_tex.Adopt(nux::CreateTextureFromFile("/usr/share/backgrounds/warty-final-ubuntu.png"));
  nux::TexCoordXForm texxform;
  auto tex_layer = std::make_shared<nux::TextureLayer>(background_tex->GetDeviceTexture(), texxform, nux::color::White);
  wt->SetWindowBackgroundPaintLayer(tex_layer.get());

  nux::NuxTimerTickSource tick_source;
  nux::animation::AnimationController animation_controller(tick_source);
  wt->Run(nullptr);

  return EXIT_SUCCESS;
}
