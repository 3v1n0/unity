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
#include "TestThreadHelper.h"

using unity::QuicklistView;
using unity::QuicklistMenuItem;
using unity::QuicklistMenuItemCheckmark;
using unity::QuicklistMenuItemLabel;
using unity::QuicklistMenuItemRadio;
using unity::QuicklistMenuItemSeparator;

static void TestMenuItemCheckmark(void);
static void TestMenuItemLabel(void);
static void TestMenuItemRadio(void);
static void TestMenuItemSeparator(void);
static void TestQuicklistMenuItem(void);

nux::WindowThread* thread = NULL;

void
TestQuicklistMenuitemsCreateSuite()
{
#define _DOMAIN "/Unit/QuicklistMenuitems"

  g_test_add_func(_DOMAIN"/MenuItemCheckmark", TestMenuItemCheckmark);
  g_test_add_func(_DOMAIN"/MenuItemLabel",     TestMenuItemLabel);
  g_test_add_func(_DOMAIN"/MenuItemRadio",     TestMenuItemRadio);
  g_test_add_func(_DOMAIN"/MenuItemSeparator", TestMenuItemSeparator);
  g_test_add_func(_DOMAIN"/QuicklistMenuItem", TestQuicklistMenuItem);
}

static void
TestMenuItemCheckmark()
{
  DbusmenuMenuitem* item = NULL;


  item = dbusmenu_menuitem_new();

  dbusmenu_menuitem_property_set(item,
                                 DBUSMENU_MENUITEM_PROP_LABEL,
                                 "Unchecked");

  dbusmenu_menuitem_property_set(item,
                                 DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE,
                                 DBUSMENU_MENUITEM_TOGGLE_CHECK);

  dbusmenu_menuitem_property_set_bool(item,
                                      DBUSMENU_MENUITEM_PROP_ENABLED,
                                      false);

  dbusmenu_menuitem_property_set_int(item,
                                     DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
                                     DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);

  QuicklistMenuItemCheckmark* qlCheckmarkItem = NULL;

  qlCheckmarkItem = new QuicklistMenuItemCheckmark(item, true);

  g_assert_cmpstr(qlCheckmarkItem->GetLabel(), == , "Unchecked");
  g_assert_cmpint(qlCheckmarkItem->GetEnabled(), == , false);
  g_assert_cmpint(qlCheckmarkItem->GetActive(), == , false);
  g_assert_cmpint(qlCheckmarkItem->GetSelectable(), == , false);
  g_assert_cmpint(qlCheckmarkItem->IsMarkupEnabled(), == , false);

  //qlCheckmarkItem->sigChanged.connect (sigc::mem_fun (pointerToCallerClass,
  //                                                    &CallerClass::RecvChanged));


  qlCheckmarkItem->Dispose();
  g_object_unref(item);
}

static void
TestMenuItemLabel()
{
  DbusmenuMenuitem* item = NULL;

  item = dbusmenu_menuitem_new();

  dbusmenu_menuitem_property_set(item,
                                 DBUSMENU_MENUITEM_PROP_LABEL,
                                 "A Label");

  dbusmenu_menuitem_property_set_bool(item,
                                      DBUSMENU_MENUITEM_PROP_ENABLED,
                                      true);

  dbusmenu_menuitem_property_set_bool(item,
                                      "unity-use-markup",
                                      true);

  QuicklistMenuItemLabel* qlLabelItem = NULL;

  qlLabelItem = new QuicklistMenuItemLabel(item, true);

  g_assert_cmpstr(qlLabelItem->GetLabel(), == , "A Label");
  g_assert_cmpint(qlLabelItem->GetEnabled(), == , true);
  g_assert_cmpint(qlLabelItem->GetSelectable(), == , true);
  g_assert_cmpint(qlLabelItem->IsMarkupEnabled(), == , true);

  //qlLabelItem->sigChanged.connect (sigc::mem_fun (pointerToCallerClass,
  //                                                &CallerClass::RecvChanged));

  qlLabelItem->Dispose();
  g_object_unref(item);
}

static void
TestMenuItemRadio()
{
  DbusmenuMenuitem* item = NULL;

  item = dbusmenu_menuitem_new();

  dbusmenu_menuitem_property_set(item,
                                 DBUSMENU_MENUITEM_PROP_LABEL,
                                 "Radio Active");

  dbusmenu_menuitem_property_set(item,
                                 DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE,
                                 DBUSMENU_MENUITEM_TOGGLE_RADIO);

  dbusmenu_menuitem_property_set_bool(item,
                                      DBUSMENU_MENUITEM_PROP_ENABLED,
                                      true);

  dbusmenu_menuitem_property_set_int(item,
                                     DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
                                     DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED);

  QuicklistMenuItemRadio* qlRadioItem = NULL;

  qlRadioItem = new QuicklistMenuItemRadio(item, true);
  qlRadioItem->EnableLabelMarkup(true);

  g_assert_cmpstr(qlRadioItem->GetLabel(), == , "Radio Active");
  g_assert_cmpint(qlRadioItem->GetEnabled(), == , true);
  g_assert_cmpint(qlRadioItem->GetActive(), == , true);
  g_assert_cmpint(qlRadioItem->GetSelectable(), == , true);
  g_assert_cmpint(qlRadioItem->IsMarkupEnabled(), == , true);

  //qlRadioItem->sigChanged.connect (sigc::mem_fun (pointerToCallerClass,
  //                                                &CallerClass::RecvChanged));

  qlRadioItem->Dispose();
  g_object_unref(item);
}

static void
TestMenuItemSeparator()
{
  DbusmenuMenuitem* item = NULL;

  item = dbusmenu_menuitem_new();

  dbusmenu_menuitem_property_set(item,
                                 "type",
                                 DBUSMENU_CLIENT_TYPES_SEPARATOR);

  dbusmenu_menuitem_property_set_bool(item,
                                      DBUSMENU_MENUITEM_PROP_ENABLED,
                                      true);

  QuicklistMenuItemSeparator* qlSeparatorItem = NULL;

  qlSeparatorItem = new QuicklistMenuItemSeparator(item, true);

  g_assert_cmpint(qlSeparatorItem->GetEnabled(), == , true);
  g_assert_cmpint(qlSeparatorItem->GetSelectable(), == , false);

  qlSeparatorItem->Dispose();
  g_object_unref(item);
}

static void
TestQuicklistMenuItem()
{
  DbusmenuMenuitem* root = dbusmenu_menuitem_new();

  dbusmenu_menuitem_set_root(root, true);

  DbusmenuMenuitem* child = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set(child, DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_MENUITEM_PROP_LABEL);
  dbusmenu_menuitem_property_set(child, DBUSMENU_MENUITEM_PROP_LABEL, "label 0");
  dbusmenu_menuitem_property_set_bool(child, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_child_append(root, child);

  child = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set(child, DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_CLIENT_TYPES_SEPARATOR);
  dbusmenu_menuitem_property_set_bool(child, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_child_append(root, child);

  child = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set(child, DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_MENUITEM_PROP_LABEL);
  dbusmenu_menuitem_property_set(child, DBUSMENU_MENUITEM_PROP_LABEL, "label 1");
  dbusmenu_menuitem_property_set_bool(child, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_child_append(root, child);

  child = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set(child, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_CHECK);
  dbusmenu_menuitem_property_set(child, DBUSMENU_MENUITEM_PROP_LABEL, "check mark 0");
  dbusmenu_menuitem_property_set_bool(child, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_property_set_int(child, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);
  dbusmenu_menuitem_child_append(root, child);

  QuicklistView* quicklist = new QuicklistView();

  quicklist->TestMenuItems(root);

  g_assert_cmpint(quicklist->GetNumItems(), == , 4);
  g_assert_cmpint(quicklist->GetNthType(0), == , unity::MENUITEM_TYPE_LABEL);
  g_assert_cmpint(quicklist->GetNthType(1), == , unity::MENUITEM_TYPE_SEPARATOR);
  g_assert_cmpint(quicklist->GetNthType(2), == , unity::MENUITEM_TYPE_LABEL);
  g_assert_cmpint(quicklist->GetNthType(3), == , unity::MENUITEM_TYPE_CHECK);

  g_assert_cmpstr(quicklist->GetNthItems(0)->GetLabel(), == , "label 0");
  g_assert_cmpstr(quicklist->GetNthItems(2)->GetLabel(), == , "label 1");
  g_assert_cmpstr(quicklist->GetNthItems(3)->GetLabel(), == , "check mark 0");

  g_assert_cmpint(quicklist->GetChildren().size(), == , 4);

  quicklist->Dispose();
}
