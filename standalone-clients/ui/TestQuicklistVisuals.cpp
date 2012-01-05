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

#include <X11/Xlib.h>

#define WIN_WIDTH  1024
#define WIN_HEIGHT  600

QuicklistView* gQuicklists[3] = {NULL, NULL, NULL};

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
createLabelItem (const gchar* string)
{
  DbusmenuMenuitem*       item  = NULL;
  QuicklistMenuItemLabel* label = NULL;

  item = dbusmenu_menuitem_new ();

  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  string);

  dbusmenu_menuitem_property_set_bool (item,
                                       DBUSMENU_MENUITEM_PROP_ENABLED,
                                       true);

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
  QuicklistMenuItemSeparator* separator = NULL;
  QuicklistMenuItemRadio*     radio     = NULL;
  QuicklistMenuItemCheckmark* checkmark = NULL;
  QuicklistMenuItemLabel*     label     = NULL;

  // radio-buttons quicklist
  gQuicklists[0] = new QuicklistView ();
  gQuicklists[0]->Reference ();
  radio = createRadioItem ("Option 01", true, false);
  gQuicklists[0]->AddMenuItem (radio);
  radio = createRadioItem ("Option 02", true, true);
  gQuicklists[0]->AddMenuItem (radio);
  radio = createRadioItem ("Option 03", true, false);
  gQuicklists[0]->AddMenuItem (radio);
  radio = createRadioItem ("Option 04", true, false);
  gQuicklists[0]->AddMenuItem (radio);
  separator = createSeparatorItem ();
  gQuicklists[0]->AddMenuItem (separator);
  label = createLabelItem ("Application Name");
  gQuicklists[0]->AddMenuItem (label);
  separator = createSeparatorItem ();
  gQuicklists[0]->AddMenuItem (separator);
  label = createLabelItem ("Remove from Quicklauncher");
  gQuicklists[0]->AddMenuItem (label);
  gQuicklists[0]->EnableQuicklistForTesting (true);
  gQuicklists[0]->SetBaseXY (45, 30);
  gQuicklists[0]->ShowWindow (true);

  // checkmarks quicklist
  gQuicklists[1] = new QuicklistView ();
  gQuicklists[1]->Reference ();
  checkmark = createCheckmarkItem ("Option 01", true, false);
  gQuicklists[1]->AddMenuItem (checkmark);
  checkmark = createCheckmarkItem ("Option 02", true, true);
  gQuicklists[1]->AddMenuItem (checkmark);
  checkmark = createCheckmarkItem ("Option 03", true, false);
  gQuicklists[1]->AddMenuItem (checkmark);
  checkmark = createCheckmarkItem ("Option 04", true, true);
  gQuicklists[1]->AddMenuItem (checkmark);
  separator = createSeparatorItem ();
  gQuicklists[1]->AddMenuItem (separator);
  label = createLabelItem ("Application Name");
  gQuicklists[1]->AddMenuItem (label);
  separator = createSeparatorItem ();
  gQuicklists[1]->AddMenuItem (separator);
  label = createLabelItem ("Remove from Quicklauncher");
  gQuicklists[1]->AddMenuItem (label);
  gQuicklists[1]->EnableQuicklistForTesting (true);
  gQuicklists[1]->SetBaseXY (520, 30);
  gQuicklists[1]->ShowWindow (true);

  // disabled items quicklist
  gQuicklists[2] = new QuicklistView ();
  gQuicklists[2]->Reference ();
  checkmark = createCheckmarkItem ("Option 01", true, false);
  gQuicklists[2]->AddMenuItem (checkmark);
  checkmark = createCheckmarkItem ("Option 02", true, false);
  gQuicklists[2]->AddMenuItem (checkmark);
  separator = createSeparatorItem ();
  gQuicklists[2]->AddMenuItem (separator);
  checkmark = createCheckmarkItem ("Option 03", false, true);
  gQuicklists[2]->AddMenuItem (checkmark);
  checkmark = createCheckmarkItem ("Option 04", false, true);
  gQuicklists[2]->AddMenuItem (checkmark);
  separator = createSeparatorItem ();
  gQuicklists[2]->AddMenuItem (separator);
  label = createLabelItem ("Application Name");
  gQuicklists[2]->AddMenuItem (label);
  separator = createSeparatorItem ();
  gQuicklists[2]->AddMenuItem (separator);
  label = createLabelItem ("Remove from Quicklauncher");
  gQuicklists[2]->AddMenuItem (label);
  gQuicklists[2]->EnableQuicklistForTesting (true);
  gQuicklists[2]->SetBaseXY (45, 290);
  gQuicklists[2]->ShowWindow (true);

  nux::ColorLayer background (nux::Color (0x772953));
  static_cast<nux::WindowThread*>(thread)->SetWindowBackgroundPaintLayer(&background);
}

int
main (int argc, char **argv)
{
  nux::WindowThread* wt = NULL;

  g_type_init ();
  g_thread_init (NULL);
  gtk_init (&argc, &argv);
  dbus_g_thread_init ();
  nux::NuxInitialize (0);

  wt = nux::CreateGUIThread (TEXT ("Unity visual Quicklist-test"),
                             WIN_WIDTH,
                             WIN_HEIGHT,
                             0,
                             &ThreadWidgetInit,
                             NULL);

  wt->Run (NULL);

  gQuicklists[0]->UnReference ();
  gQuicklists[1]->UnReference ();
  gQuicklists[2]->UnReference ();
  delete wt;

  return 0;
}
