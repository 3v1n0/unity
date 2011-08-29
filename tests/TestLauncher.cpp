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

#include "LauncherController.h"
#include "Launcher.h"
#include "BackgroundEffectHelper.h"
#include <dbus/dbus-glib.h>

using namespace unity::ui;

static Launcher *launcher;
static LauncherController *controller;

void ThreadWidgetInit(nux::NThread* thread, void* InitData)
{
  nux::BaseWindow* launcherWindow = new nux::BaseWindow(TEXT("LauncherWindow"));
  launcherWindow->SinkReference();
  launcherWindow->SetGeometry (nux::Geometry(0, 0, 300, 800));

  launcher = new Launcher(launcherWindow);

  nux::HLayout* layout = new nux::HLayout();
  layout->AddView(launcher, 1);
  layout->SetContentDistribution(nux::eStackLeft);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);

  controller = new LauncherController(launcher);

  launcherWindow->SetLayout(layout);
  launcherWindow->SetBackgroundColor(nux::Color(0x00000000));
  launcherWindow->ShowWindow(true);
  launcherWindow->SetEnterFocusInputArea(launcher);

  launcher->SetIconSize(54, 48);
  launcher->SetBacklightMode(Launcher::BACKLIGHT_ALWAYS_ON);

  launcher->SetHideMode(Launcher::LAUNCHER_HIDE_DODGE_WINDOWS);
  launcher->SetLaunchAnimation(Launcher::LAUNCH_ANIMATION_PULSE);
  launcher->SetUrgentAnimation(Launcher::URGENT_ANIMATION_WIGGLE);
}

int main(int argc, char** argv)
{
  g_type_init();
  g_thread_init(NULL);
  gtk_init(&argc, &argv);

  dbus_g_thread_init();

  nux::NuxInitialize(0);

  BackgroundEffectHelper::blur_type = unity::BLUR_NONE;
  nux::WindowThread* wt = nux::CreateGUIThread(TEXT("Unity Switcher"), 300, 800, 0, &ThreadWidgetInit, 0);

  wt->Run(NULL);
  delete wt;
  return 0;
}