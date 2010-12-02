/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 */

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/WindowThread.h"

#include "QuicklistView.h"
#include "QuicklistMenuItem.h"
#include "QuicklistMenuItemLabel.h"
#include "QuicklistMenuItemSeparator.h"
#include "QuicklistMenuItemCheckmark.h"
#include "QuicklistMenuItemRadio.h"

#include "EventFaker.h"

#define WIN_WIDTH  400
#define WIN_HEIGHT 300

/*Add first UI test which is creating ech of the QL menuitems, connecting to their
 dbusmenuitem's activate signal, adding the QL menu item to a nux window, and
 then sending a fake event using EventFaker and making sure that the
 dbusmenuitem's activate signal was called.*/

void
ThreadWidgetInit (nux::NThread* thread,
                  void*         initData)
{
  nux::VLayout* layout = new nux::VLayout (TEXT(""), NUX_TRACKER_LOCATION);
  QuicklistView::QuicklistView* quicklist = new QuicklistView::QuicklistView ();

  QuicklistMenuItemSeparator* item = new QuicklistMenuItemSeparator (menu_item, NUX_TRACKER_LOCATION);
  quicklist->AddMenuItem (item);

  QuicklistMenuItemCheckmark* item = new QuicklistMenuItemCheckmark (menu_item, NUX_TRACKER_LOCATION);
  quicklist->AddMenuItem (item);

  QuicklistMenuItemRadio* item = new QuicklistMenuItemRadio (menu_item, NUX_TRACKER_LOCATION);
  quicklist->AddMenuItem (item);

  QuicklistMenuItemLabel* item = new QuicklistMenuItemLabel (menu_item, NUX_TRACKER_LOCATION);
  quicklist->AddMenuItem (item);

  quicklist->ShowQuicklistWithTipAt (tip_x, tip_y);
  quicklist->EnableInputWindow (true, 1);
  quicklist->GrabPointer ();
  //nux::GetWindowCompositor ().SetAlwaysOnFrontWindow (_quicklist);
  quicklist->NeedRedraw ();


  layout->AddView (quicklist, 1, nux::eCenter, nux::eFix);
  layout->SetContentDistribution (nux::eStackCenter);

  nux::GetGraphicsThread()->SetLayout (layout);
}

/*int
main (int argc, char **argv)
{
  g_type_init ();
  g_thread_init (NULL);
  gtk_init (&argc, &argv);

  dbus_g_thread_init ();

  nux::NuxInitialize(0);

  nux::WindowThread* wt = NULL;
  wt = nux::CreateGUIThread (TEXT ("Unity Quicklist"),
                             WIN_WIDTH,
                             WIN_HEIGHT,
                             0,
                             &ThreadWidgetInit,
                             0);
  
  wt->Run (NULL);

  delete wt;

  return 0;
}*/
