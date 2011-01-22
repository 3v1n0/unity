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
#include "Nux/TextureArea.h"
#include "Nux/WindowThread.h"
#include "NuxGraphics/GraphicsEngine.h"
#include <gtk/gtk.h>

#include "../src/ubus-server.h"

#include "PlacesView.h"
#include "UBusMessages.h"

void ThreadWidgetInit(nux::NThread* thread, void* InitData)
{
  nux::VLayout *layout = new nux::VLayout(TEXT(""), NUX_TRACKER_LOCATION);
  PlacesView *view = new PlacesView ();

  view->SetMinMaxSize(1024, 600);
  layout->AddView(view, 1, nux::eCenter, nux::eFix);
  layout->SetContentDistribution(nux::eStackCenter);

  nux::GetGraphicsThread()->SetLayout (layout);
}

int main(int argc, char **argv)
{
  g_type_init ();
  g_thread_init (NULL);
  gtk_init (&argc, &argv);

  nux::NuxInitialize(0);

  nux::WindowThread* wt = nux::CreateGUIThread("Unity Places",
                                               1024, 600, 0, &ThreadWidgetInit, 0);
  
  wt->Run(NULL);
  delete wt;
  return 0;
}
