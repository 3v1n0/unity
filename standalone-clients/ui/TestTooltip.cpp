/*
 * Copyright (C) 2012 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <glib.h>
#include <gtk/gtk.h>

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/WindowThread.h"

#include "Tooltip.h"

#define WIN_WIDTH  200
#define WIN_HEIGHT 75

using namespace unity;

void
ThreadWidgetInit (nux::NThread* thread,
                  void*         initData)
{
  Tooltip* ttip = new Tooltip();
  ttip->SetText("Unity Tooltip");
  ttip->ShowTooltipWithTipAt(15, 30);

  nux::ColorLayer background(nux::Color (0x772953));
  static_cast<nux::WindowThread*>(thread)->SetWindowBackgroundPaintLayer(&background);
}

int
main (int argc, char **argv)
{
  nux::WindowThread* wt = NULL;

  gtk_init(&argc, &argv);
  nux::NuxInitialize(0);

  wt = nux::CreateGUIThread(TEXT ("Unity visual Tooltip test"),
                            WIN_WIDTH,
                            WIN_HEIGHT,
                            0,
                            &ThreadWidgetInit,
                            NULL);

  wt->Run(NULL);
  delete wt;

  return 0;
}
