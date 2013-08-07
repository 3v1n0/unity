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
#include "Nux/VLayout.h"
#include "Nux/WindowThread.h"
#include "NuxGraphics/GraphicsEngine.h"
#include <NuxCore/Logger.h>

#include "dash/DashController.h"
#include "dash/DashView.h"
#include "launcher/FavoriteStoreGSettings.h"
#include "launcher/Launcher.h"
#include "launcher/LauncherController.h"
#include "panel/PanelController.h"
#include "panel/PanelView.h"
#include "unity-shared/BGHash.h"
#include "unity-shared/BackgroundEffectHelper.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/FontSettings.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/ThumbnailGenerator.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/UnitySettings.h"

namespace
{
  static int display_width = 1200;
  static int display_height = 720;
  static gboolean no_window_decorations = FALSE;
  static gboolean force_tv = FALSE;

  static GOptionEntry entries[] =
  {
    {"width", 'w', 0, G_OPTION_ARG_INT, &display_width, "Display width", NULL},
    {"height", 'h', 0, G_OPTION_ARG_INT, &display_height, "Display height", NULL},
    {"no-window-decorations", 'd', 0, G_OPTION_ARG_NONE, &no_window_decorations, "Disables the window decorations", NULL},
    {"force-tv", 't', 0, G_OPTION_ARG_NONE, &force_tv, "Forces the TV interface", NULL},
    {NULL}
  };
}

using namespace unity;

class UnityStandalone
{
public:
  UnityStandalone ();
  ~UnityStandalone ();

  static void InitWindowThread (nux::NThread* thread, void* InitData);
  void Init ();

  launcher::Controller::Ptr launcher_controller;
  dash::Controller::Ptr dash_controller;
  panel::Controller::Ptr panel_controller;
};

UnityStandalone::UnityStandalone ()
{
}

UnityStandalone::~UnityStandalone ()
{
}

void UnityStandalone::Init ()
{
  auto xdnd_manager = std::make_shared<XdndManager>();
  auto edge_barriers = std::make_shared<ui::EdgeBarrierController>();
  launcher_controller = std::make_shared<launcher::Controller>(xdnd_manager, edge_barriers);
  panel_controller = std::make_shared<panel::Controller>();
  dash_controller = std::make_shared<dash::Controller>();

  dash_controller->launcher_width = launcher_controller->launcher().GetAbsoluteWidth() - 1;
  panel_controller->launcher_width = launcher_controller->launcher().GetAbsoluteWidth() - 1;
}

void UnityStandalone::InitWindowThread(nux::NThread* thread, void* InitData)
{
  UnityStandalone *self = static_cast<UnityStandalone*>(InitData);
  self->Init();
}


class UnityStandaloneTV
{
public:
  UnityStandaloneTV();
  ~UnityStandaloneTV();

  static void InitWindowThread (nux::NThread* thread, void* InitData);
  void Init();

  launcher::Controller::Ptr launcher_controller;
  dash::Controller::Ptr dash_controller;
};

UnityStandaloneTV::UnityStandaloneTV() {};
UnityStandaloneTV::~UnityStandaloneTV() {};

void UnityStandaloneTV::Init()
{
  auto xdnd_manager = std::make_shared<XdndManager>();
  auto edge_barriers = std::make_shared<ui::EdgeBarrierController>();
  launcher_controller = std::make_shared<launcher::Controller>(xdnd_manager, edge_barriers);
  dash_controller = std::make_shared<dash::Controller>();
  dash_controller->launcher_width = launcher_controller->launcher().GetAbsoluteWidth() - 1;

  UBusManager().SendMessage(UBUS_DASH_EXTERNAL_ACTIVATION, nullptr);
}

void UnityStandaloneTV::InitWindowThread(nux::NThread* thread, void* InitData)
{
  UnityStandaloneTV *self = static_cast<UnityStandaloneTV*>(InitData);
  self->Init();
}

int main(int argc, char **argv)
{
  nux::WindowThread* wt = NULL;
  GError *error = NULL;
  GOptionContext *context;

  nux::NuxInitialize(0);
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));
  
  context = g_option_context_new("- Unity standalone");
  g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group(context, gtk_get_option_group(TRUE));
  g_option_context_parse(context, &argc, &argv, &error);
  if (error != NULL)
  {
    g_print("Option parsiong failed: %s\n", error->message);
    g_error_free(error);
    exit(1);
  }

  gtk_init(&argc, &argv);

  BGHash bghash;
  FontSettings font_settings;

  // The instances for the pseudo-singletons.
  Settings settings;
  settings.is_standalone = true;
  if (force_tv) Settings::Instance().form_factor(FormFactor::TV);

  dash::Style dash_style;
  panel::Style panel_style;
  unity::ThumbnailGenerator thumbnail_generator;

  internal::FavoriteStoreGSettings favorite_store;
  BackgroundEffectHelper::blur_type = BLUR_NONE;

  if (!force_tv)
  {
    UnityStandalone *standalone_runner = new UnityStandalone();
    wt = nux::CreateNuxWindow("standalone-unity",
                              display_width, display_height,
                              (no_window_decorations) ? nux::WINDOWSTYLE_NOBORDER : nux::WINDOWSTYLE_NORMAL,
                              0, /* no parent */
                              false,
                              &UnityStandalone::InitWindowThread,
                              standalone_runner);
  }
  else
  {
    //TODO - we should be able to pass in a monitor so that we can make the window
    //the size of the monitor and position the window on the monitor correctly.
    UnityStandaloneTV *standalone_runner = new UnityStandaloneTV();
    wt = nux::CreateNuxWindow("standalone-unity-tv",
                              display_width, display_height,
                              (no_window_decorations) ? nux::WINDOWSTYLE_NOBORDER : nux::WINDOWSTYLE_NORMAL,
                              0, /* no parent */
                              false,
                              &UnityStandaloneTV::InitWindowThread,
                              standalone_runner);
  }
  wt->Run(NULL);
  delete wt;
  return 0;
}
