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
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <glib.h>
#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>

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
#include <X11/Xlib.h>

#define WIN_WIDTH  400
#define WIN_HEIGHT 400

gboolean gResult[3] = {false, false, false};

QuicklistView*                gQuicklist = NULL;
QuicklistMenuItemCheckmark*   gCheckmark = NULL;
QuicklistMenuItemRadio*       gRadio     = NULL;
QuicklistMenuItemLabel*       gLabel     = NULL;
QuicklistMenuItemSeparator*   gSeparator = NULL;

void
activatedCallback (DbusmenuMenuitem* item,
                   int               time,
                   gpointer          data)
{
  gboolean* result = (gboolean*) data;

  *result = true;

  g_print ("Quicklist-item activated\n");
}

QuicklistMenuItemRadio*
createRadioItem (const gchar* label,
                 gboolean     enabled,
                 gboolean     checked)
{
  DbusmenuMenuitem*       item  = NULL;
  QuicklistMenuItemRadio* radio = NULL;

  item = dbusmenu_menuitem_new ();

  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  label);

  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE,
                                  DBUSMENU_MENUITEM_TOGGLE_RADIO);

  dbusmenu_menuitem_property_set_bool (item,
                                       DBUSMENU_MENUITEM_PROP_ENABLED,
                                       enabled);

  dbusmenu_menuitem_property_set_int (item,
                                      DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
                                      (checked ?
                                        DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED :
                                        DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED
                                      ));

  radio = new QuicklistMenuItemRadio (item, true);
    
  return radio;
}

QuicklistMenuItemCheckmark*
createCheckmarkItem (const gchar* label,
                     gboolean     enabled,
                     gboolean     checked)
{
  DbusmenuMenuitem*           item      = NULL;
  QuicklistMenuItemCheckmark* checkmark = NULL;

  item = dbusmenu_menuitem_new ();

  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  label);

  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE,
                                  DBUSMENU_MENUITEM_TOGGLE_CHECK);

  dbusmenu_menuitem_property_set_bool (item,
                                       DBUSMENU_MENUITEM_PROP_ENABLED,
                                       enabled);

  dbusmenu_menuitem_property_set_int (item,
                                      DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
                                      (checked ?
                                        DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED :
                                        DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED
                                      ));

  checkmark = new QuicklistMenuItemCheckmark (item, true);
    
  return checkmark;
}

QuicklistMenuItemLabel*
createLabelItem (const gchar* string, bool enabled = true)
{
  DbusmenuMenuitem*       item  = NULL;
  QuicklistMenuItemLabel* label = NULL;

  item = dbusmenu_menuitem_new ();

  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  string);

  dbusmenu_menuitem_property_set_bool (item,
                                       DBUSMENU_MENUITEM_PROP_ENABLED,
                                       enabled);

  label = new QuicklistMenuItemLabel (item, true);

  return label;
}

QuicklistMenuItemSeparator*
createSeparatorItem ()
{
  DbusmenuMenuitem*           item      = NULL;
  QuicklistMenuItemSeparator* separator = NULL;

  item = dbusmenu_menuitem_new ();

  dbusmenu_menuitem_property_set_bool (item,
                                       DBUSMENU_MENUITEM_PROP_ENABLED,
                                       true);

  separator = new QuicklistMenuItemSeparator (item, true);

  return separator;
}

void
ThreadWidgetInit (nux::NThread* thread,
                  void*         initData)
{
  gQuicklist = new QuicklistView ();
  gQuicklist->Reference ();

  gLabel = createLabelItem("Item1, normal");
  gQuicklist->AddMenuItem (gLabel);

  gSeparator = createSeparatorItem();
  gQuicklist->AddMenuItem (gSeparator);

  gRadio = createRadioItem("Item2, radio, checked", true, true);
  gQuicklist->AddMenuItem (gRadio);

  gRadio = createRadioItem("Item3, radio, unchecked", true, false);
  gQuicklist->AddMenuItem (gRadio);

  gRadio = createRadioItem("Item4, disabled radio, checked", false, true);
  gQuicklist->AddMenuItem (gRadio);

  gRadio = createRadioItem("Item5, disabled radio, unchecked", false, false);
  gQuicklist->AddMenuItem (gRadio);

  gCheckmark = createCheckmarkItem("Item6, checkmark, checked", true, true);
  gQuicklist->AddMenuItem (gCheckmark);

  gCheckmark = createCheckmarkItem("Item7, checkmark, unchecked", true, false);
  gQuicklist->AddMenuItem (gCheckmark);

  gCheckmark = createCheckmarkItem("Item8, disabled checkmark, checked", false, true);
  gQuicklist->AddMenuItem (gCheckmark);

  gCheckmark = createCheckmarkItem("Item9, disabled checkmark, unchecked", false, false);
  gQuicklist->AddMenuItem (gCheckmark);

  gLabel = createLabelItem("Item1, disabled", false);
  gQuicklist->AddMenuItem (gLabel);

  gQuicklist->EnableQuicklistForTesting (true);

  gQuicklist->SetBaseXY (0, 0);
  gQuicklist->ShowWindow (true);

  nux::ColorLayer background (nux::Color (0x772953));
  static_cast<nux::WindowThread*>(thread)->SetWindowBackgroundPaintLayer(&background);
}

void
ControlThread (nux::NThread* thread,
               void*         data)
{
  // sleep for 3 seconds
  nux::SleepForMilliseconds (3000);
  printf ("ControlThread successfully started\n");

  nux::WindowThread* mainWindowThread = NUX_STATIC_CAST (nux::WindowThread*, 
                                                         data);

  mainWindowThread->SetFakeEventMode (true);
  Display* display = mainWindowThread->GetWindow ().GetX11Display ();

  // assemble first button-click event
  XEvent buttonPressEvent;
  buttonPressEvent.xbutton.type        = ButtonPress;
  buttonPressEvent.xbutton.serial      = 0;
  buttonPressEvent.xbutton.send_event  = False;
  buttonPressEvent.xbutton.display     = display;
  buttonPressEvent.xbutton.window      = 0;
  buttonPressEvent.xbutton.root        = 0;
  buttonPressEvent.xbutton.subwindow   = 0;
  buttonPressEvent.xbutton.time        = CurrentTime;
  buttonPressEvent.xbutton.x           = 50;
  buttonPressEvent.xbutton.y           = 30;
  buttonPressEvent.xbutton.x_root      = 0;
  buttonPressEvent.xbutton.y_root      = 0;
  buttonPressEvent.xbutton.state       = 0;
  buttonPressEvent.xbutton.button      = Button1;
  buttonPressEvent.xbutton.same_screen = True;

  mainWindowThread->PumpFakeEventIntoPipe (mainWindowThread,
                                           (XEvent*) &buttonPressEvent);

  while (!mainWindowThread->ReadyForNextFakeEvent ())
    nux::SleepForMilliseconds (10);

  XEvent buttonReleaseEvent;
  buttonReleaseEvent.xbutton.type        = ButtonRelease;
  buttonReleaseEvent.xbutton.serial      = 0;
  buttonReleaseEvent.xbutton.send_event  = False;
  buttonReleaseEvent.xbutton.display     = display;
  buttonReleaseEvent.xbutton.window      = 0;
  buttonReleaseEvent.xbutton.root        = 0;
  buttonReleaseEvent.xbutton.subwindow   = 0;
  buttonReleaseEvent.xbutton.time        = CurrentTime;
  buttonReleaseEvent.xbutton.x           = 50;
  buttonReleaseEvent.xbutton.y           = 30;
  buttonReleaseEvent.xbutton.x_root      = 0;
  buttonReleaseEvent.xbutton.y_root      = 0;
  buttonReleaseEvent.xbutton.state       = 0;
  buttonReleaseEvent.xbutton.button      = Button1;
  buttonReleaseEvent.xbutton.same_screen = True;

  mainWindowThread->PumpFakeEventIntoPipe (mainWindowThread,
                                           (XEvent*) &buttonReleaseEvent);

  while (!mainWindowThread->ReadyForNextFakeEvent ())
    nux::SleepForMilliseconds (10);

  // assemble second button-click event
  buttonPressEvent.xbutton.time = CurrentTime;
  buttonPressEvent.xbutton.x    = 50;
  buttonPressEvent.xbutton.y    = 50;
  mainWindowThread->PumpFakeEventIntoPipe (mainWindowThread,
                                           (XEvent*) &buttonPressEvent);
  while (!mainWindowThread->ReadyForNextFakeEvent ())
    nux::SleepForMilliseconds (10);

  buttonReleaseEvent.xbutton.time = CurrentTime;
  buttonReleaseEvent.xbutton.x    = 50;
  buttonReleaseEvent.xbutton.y    = 50;
  mainWindowThread->PumpFakeEventIntoPipe (mainWindowThread,
                                           (XEvent*) &buttonReleaseEvent);
  while (!mainWindowThread->ReadyForNextFakeEvent ())
    nux::SleepForMilliseconds (10);

  // assemble third button-click event
  buttonPressEvent.xbutton.time = CurrentTime;
  buttonPressEvent.xbutton.x    = 50;
  buttonPressEvent.xbutton.y    = 70;
  mainWindowThread->PumpFakeEventIntoPipe (mainWindowThread,
                                           (XEvent*) &buttonPressEvent);
  while (!mainWindowThread->ReadyForNextFakeEvent ())
    nux::SleepForMilliseconds (10);

  buttonReleaseEvent.xbutton.time = CurrentTime;
  buttonReleaseEvent.xbutton.x    = 50;
  buttonReleaseEvent.xbutton.y    = 70;
  mainWindowThread->PumpFakeEventIntoPipe (mainWindowThread,
                                           (XEvent*) &buttonReleaseEvent);
  while (!mainWindowThread->ReadyForNextFakeEvent ())
    nux::SleepForMilliseconds (10);

  mainWindowThread->SetFakeEventMode (false);
}

int
main (int argc, char **argv)
{
  nux::WindowThread* wt = NULL;
  nux::SystemThread* st = NULL;

  g_type_init ();
  g_thread_init (NULL);
  gtk_init (&argc, &argv);
  dbus_g_thread_init ();
  nux::NuxInitialize (0);

  wt = nux::CreateGUIThread (TEXT ("Unity Quicklist"),
                             WIN_WIDTH,
                             WIN_HEIGHT,
                             0,
                             &ThreadWidgetInit,
                             NULL);

  st = nux::CreateSystemThread (NULL, ControlThread, wt);
  if (st)
    st->Start (NULL);

  wt->Run (NULL);

  gQuicklist->UnReference ();
  delete st;
  delete wt;

  g_assert_cmpint (gResult[0], ==, true);
  g_assert_cmpint (gResult[1], ==, true);
  g_assert_cmpint (gResult[2], ==, true);

  return 0;
}
