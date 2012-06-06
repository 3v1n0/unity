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

#include "Nux/Nux.h"
#include "Nux/Button.h"
#include "Nux/VLayout.h"
#include "Nux/HLayout.h"
#include "Nux/WindowThread.h"
#include "Nux/CheckBox.h"
#include "Nux/SpinBox.h"
#include "Nux/EditTextBox.h"
#include "Nux/StaticText.h"
#include "Nux/RangeValueInteger.h"
#include "NuxGraphics/GraphicsEngine.h"
#include <gtk/gtk.h>

#include "unity-shared/BackgroundEffectHelper.h"
#include "FavoriteStoreGSettings.h"
#include "LauncherController.h"
#include "Launcher.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UnitySettings.h"
#include <dbus/dbus-glib.h>

using namespace unity;

static launcher::Controller::Ptr controller;

void ThreadWidgetInit(nux::NThread* thread, void* InitData)
{
//  launcherWindow->SetGeometry (nux::Geometry(0, 0, 300, 800));
  controller.reset(new launcher::Controller(0));
}

int main(int argc, char** argv)
{
  g_type_init();
  
  gtk_init(&argc, &argv);

  dbus_g_thread_init();

  nux::NuxInitialize(0);

  GeisAdapter geis_adapter;
  unity::Settings settings;
  panel::Style panel_style;
  internal::FavoriteStoreGSettings favorite_store;

  BackgroundEffectHelper::blur_type = BLUR_NONE;
  nux::WindowThread* wt = nux::CreateGUIThread(TEXT("Unity Switcher"), 300, 800, 0, &ThreadWidgetInit, 0);

  wt->Run(NULL);
  delete wt;
  return 0;
}
