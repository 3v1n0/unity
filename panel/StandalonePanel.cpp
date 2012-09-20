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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

#include <Nux/Nux.h>
#include <Nux/VLayout.h>
#include <Nux/HLayout.h>
#include <Nux/WindowThread.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <NuxCore/Logger.h>
#include <gtk/gtk.h>

#include "unity-shared/UnitySettings.h"
#include "unity-shared/PanelStyle.h"
#include "PanelView.h"

void ThreadWidgetInit(nux::NThread* thread, void* InitData)
{
  nux::VLayout* layout = new nux::VLayout(TEXT(""), NUX_TRACKER_LOCATION);
  unity::PanelView* view = new unity::PanelView();

  //view->SetMinMaxSize(1024, 24);
  view->SetPrimary(true);
  layout->AddView(view, 1, nux::eCenter, nux::eFull);
  layout->SetContentDistribution(nux::eStackCenter);

  nux::GetWindowThread()->SetLayout(layout);
}

int main(int argc, char** argv)
{
  g_type_init();
  gtk_init(&argc, &argv);
  nux::NuxInitialize(0);
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));
  
  // The instances for the pseudo-singletons.
  unity::Settings settings;
  unity::panel::Style panel_style;

  nux::WindowThread* wt = nux::CreateGUIThread(TEXT("Unity Panel"), 1024, 24, 0, &ThreadWidgetInit, 0);

  wt->Run(NULL);
  delete wt;
  return 0;
}
