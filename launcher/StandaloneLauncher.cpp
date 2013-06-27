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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 *
 */

#include <Nux/Nux.h>
#include <Nux/NuxTimerTickSource.h>
#include <NuxCore/AnimationController.h>
#include <NuxCore/Logger.h>
#include <gtk/gtk.h>

#include "unity-shared/BackgroundEffectHelper.h"
#include "FavoriteStoreGSettings.h"
#include "LauncherController.h"
#include "Launcher.h"
#include "unity-shared/IconRenderer.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/UScreen.h"

using namespace unity;

namespace
{
const nux::Size win_size(1024, 768);
const nux::Color bg_color(95/255.0f, 18/255.0f, 45/255.0f, 1.0f);
}

struct LauncherWindow
{
  LauncherWindow()
    : wt(nux::CreateGUIThread("Unity Launcher", win_size.width, win_size.height, 0, &LauncherWindow::ThreadWidgetInit, this))
    , animation_controller(tick_source)
  {}

  void Show()
  {
    wt->Run(nullptr);
  }

private:
  void SetupBackground()
  {
    nux::ObjectPtr<nux::BaseTexture> background_tex;
    background_tex.Adopt(nux::CreateTextureFromFile("/usr/share/backgrounds/warty-final-ubuntu.png"));
    nux::TexCoordXForm texxform;
    auto tex_layer = std::make_shared<nux::TextureLayer>(background_tex->GetDeviceTexture(), texxform, nux::color::White);
    wt->SetWindowBackgroundPaintLayer(tex_layer.get());
  }

  void Init()
  {
    SetupBackground();
    controller.reset(new launcher::Controller(std::make_shared<XdndManager>()));

    UScreen* uscreen = UScreen::GetDefault();
    std::vector<nux::Geometry> fake_monitor({nux::Geometry(0, 0, win_size.width, win_size.height)});
    uscreen->changed.emit(0, fake_monitor);
    uscreen->changed.clear();
    controller->launcher().Resize(nux::Point(), win_size.height);

    wt->window_configuration.connect([this] (int x, int y, int w, int h) {
      controller->launcher().Resize(nux::Point(), h);
    });
  }

  static void ThreadWidgetInit(nux::NThread* thread, void* self)
  {
    static_cast<LauncherWindow*>(self)->Init();
  }

  internal::FavoriteStoreGSettings favorite_store;
  unity::Settings settings;
  panel::Style panel_style;
  std::shared_ptr<nux::WindowThread> wt;
  nux::NuxTimerTickSource tick_source;
  nux::animation::AnimationController animation_controller;
  launcher::Controller::Ptr controller;
};

int main(int argc, char** argv)
{
  gtk_init(&argc, &argv);
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));
  nux::NuxInitialize(0);

  LauncherWindow lc;
  lc.Show();

  return 0;
}
