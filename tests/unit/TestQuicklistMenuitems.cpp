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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 *
 */

#include "config.h"

#include "QuicklistMenuItem.h"
#include "QuicklistMenuItemCheckmark.h"
#include "QuicklistMenuItemLabel.h"
#include "QuicklistMenuItemRadio.h"
#include "QuicklistMenuItemSeparator.h"

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/HLayout.h"
#include "Nux/WindowThread.h"
#include "Nux/WindowCompositor.h"
#include "Nux/BaseWindow.h"

#include "QuicklistView.h"

static void TestMenuItemCheckmark (void);
static void TestMenuItemLabel     (void);
static void TestMenuItemRadio     (void);
static void TestMenuItemSeparator (void);
static void TestQuicklistMenuItem (void);

void
TestQuicklistMenuitemsCreateSuite ()
{
#define _DOMAIN "/Unit/QuicklistMenuitems"

  g_test_add_func (_DOMAIN"/MenuItemCheckmark", TestMenuItemCheckmark);
  g_test_add_func (_DOMAIN"/MenuItemLabel",     TestMenuItemLabel);
  g_test_add_func (_DOMAIN"/MenuItemRadio",     TestMenuItemRadio);
  g_test_add_func (_DOMAIN"/MenuItemSeparator", TestMenuItemSeparator);
  g_test_add_func (_DOMAIN"/QuicklistMenuItem", TestQuicklistMenuItem);
}

static void
TestMenuItemCheckmark ()
{
  DbusmenuMenuitem* item = NULL;

  item = dbusmenu_menuitem_new ();

  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  "Unchecked");

  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE,
                                  DBUSMENU_MENUITEM_TOGGLE_CHECK);

  dbusmenu_menuitem_property_set_bool (item,
                                       DBUSMENU_MENUITEM_PROP_ENABLED,
                                       true);

  dbusmenu_menuitem_property_set_int (item,
                                      DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
                                      DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);

  QuicklistMenuItemCheckmark* qlCheckmarkItem = NULL;

  qlCheckmarkItem = new QuicklistMenuItemCheckmark (item, true);

  g_assert_cmpstr (qlCheckmarkItem->GetLabel (), ==, "Unchecked");
  g_assert_cmpint (qlCheckmarkItem->GetEnabled (), ==, false);
  g_assert_cmpint (qlCheckmarkItem->GetActive (), ==, true);

  //qlCheckmarkItem->sigChanged.connect (sigc::mem_fun (pointerToCallerClass,
  //                                                    &CallerClass::RecvChanged));

  delete qlCheckmarkItem;
  g_object_unref (item);
}

static void
TestMenuItemLabel ()
{
  DbusmenuMenuitem* item = NULL;

  item = dbusmenu_menuitem_new ();

  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  "A Label");

  dbusmenu_menuitem_property_set_bool (item,
                                       DBUSMENU_MENUITEM_PROP_ENABLED,
                                       true);

  QuicklistMenuItemLabel* qlLabelItem = NULL;

  qlLabelItem = new QuicklistMenuItemLabel (item, true);

  g_assert_cmpstr (qlLabelItem->GetLabel (), ==, "A Label");
  g_assert_cmpint (qlLabelItem->GetEnabled (), ==, true);

  //qlLabelItem->sigChanged.connect (sigc::mem_fun (pointerToCallerClass,
  //                                                &CallerClass::RecvChanged));
    
  delete qlLabelItem;
  g_object_unref (item);
}

static void
TestMenuItemRadio ()
{
  DbusmenuMenuitem* item = NULL;

  item = dbusmenu_menuitem_new ();

  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  "Radio Active");

  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE,
                                  DBUSMENU_MENUITEM_TOGGLE_RADIO);

  dbusmenu_menuitem_property_set_bool (item,
                                       DBUSMENU_MENUITEM_PROP_ENABLED,
                                       true);

  dbusmenu_menuitem_property_set_int (item,
                                      DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
                                      DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED);

  QuicklistMenuItemRadio* qlRadioItem = NULL;

  qlRadioItem = new QuicklistMenuItemRadio (item, true);

  g_assert_cmpstr (qlRadioItem->GetLabel (), ==, "Radio Active");
  g_assert_cmpint (qlRadioItem->GetEnabled (), ==, true);
  g_assert_cmpint (qlRadioItem->GetActive (), ==, true);

  //qlRadioItem->sigChanged.connect (sigc::mem_fun (pointerToCallerClass,
  //                                                &CallerClass::RecvChanged));
    
  delete qlRadioItem;
  g_object_unref (item);
}

static void
TestMenuItemSeparator ()
{
  DbusmenuMenuitem* item = NULL;

  item = dbusmenu_menuitem_new ();

  dbusmenu_menuitem_property_set (item,
                                  "type",
                                  DBUSMENU_CLIENT_TYPES_SEPARATOR);

  dbusmenu_menuitem_property_set_bool (item,
                                       DBUSMENU_MENUITEM_PROP_ENABLED,
                                       true);

  QuicklistMenuItemSeparator* qlSeparatorItem = NULL;

  qlSeparatorItem = new QuicklistMenuItemSeparator (item, true);

  g_assert_cmpint (qlSeparatorItem->GetEnabled (), ==, true);

  delete qlSeparatorItem;
  g_object_unref (item);
}

static void
TestQuicklistMenuItem ()
{
  DbusmenuMenuitem* root = dbusmenu_menuitem_new ();
  
  dbusmenu_menuitem_set_root (root, true);

  DbusmenuMenuitem* child = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set (child, DBUSMENU_MENUITEM_PROP_LABEL, "Label");
  dbusmenu_menuitem_property_set_bool (child, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_child_append (root, child);
  
  QuicklistView* quicklist = new QuicklistView ();
  
  quicklist->TestMenuItems (root);
  
  delete quicklist;
}
