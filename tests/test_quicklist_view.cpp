/*
 * Copyright 2010-2012 Canonical Ltd.
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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 *              Mirco Müller <mirco.mueller@canonical.com>
 */

#include <gmock/gmock.h>
#include <libdbusmenu-glib/client.h>

#include "QuicklistView.h"
#include "QuicklistMenuItemCheckmark.h"
#include "QuicklistMenuItemLabel.h"
#include "QuicklistMenuItemRadio.h"
#include "QuicklistMenuItemSeparator.h"
#include "unity-shared/UnitySettings.h"

using namespace unity;
using namespace testing;

namespace
{

struct TestQuicklistView : public Test
{
  TestQuicklistView()
    : quicklist(new QuicklistView())
  {}

  void AddMenuItems(glib::Object<DbusmenuMenuitem> const& root)
  {
    quicklist->RemoveAllMenuItem();

    if (!root)
      return;

    for (GList* child = dbusmenu_menuitem_get_children(root); child; child = child->next)
    {
      glib::Object<DbusmenuMenuitem> item(static_cast<DbusmenuMenuitem*>(child->data), glib::AddRef());
      const gchar* type = dbusmenu_menuitem_property_get(item, DBUSMENU_MENUITEM_PROP_TYPE);
      const gchar* toggle_type = dbusmenu_menuitem_property_get(item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE);

      if (g_strcmp0(type, DBUSMENU_CLIENT_TYPES_SEPARATOR) == 0)
      {
        QuicklistMenuItemSeparator* qlitem = new QuicklistMenuItemSeparator(item, NUX_TRACKER_LOCATION);
        quicklist->AddMenuItem(qlitem);
      }
      else if (g_strcmp0(toggle_type, DBUSMENU_MENUITEM_TOGGLE_CHECK) == 0)
      {
        QuicklistMenuItemCheckmark* qlitem = new QuicklistMenuItemCheckmark(item, NUX_TRACKER_LOCATION);
        quicklist->AddMenuItem(qlitem);
      }
      else if (g_strcmp0(toggle_type, DBUSMENU_MENUITEM_TOGGLE_RADIO) == 0)
      {
        QuicklistMenuItemRadio* qlitem = new QuicklistMenuItemRadio(item, NUX_TRACKER_LOCATION);
        quicklist->AddMenuItem(qlitem);
      }
      else //if (g_strcmp0 (type, DBUSMENU_MENUITEM_PROP_LABEL) == 0)
      {
        QuicklistMenuItemLabel* qlitem = new QuicklistMenuItemLabel(item, NUX_TRACKER_LOCATION);
        quicklist->AddMenuItem(qlitem);
      }
    }
  }

  unity::Settings unity_settings;
  nux::ObjectPtr<QuicklistView> quicklist;
};

TEST_F(TestQuicklistView, AddItems)
{
  glib::Object<DbusmenuMenuitem> root(dbusmenu_menuitem_new());

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

  AddMenuItems(root);

  ASSERT_EQ(quicklist->GetChildren().size(), 4);
  ASSERT_EQ(quicklist->GetNumItems(), 4);
  EXPECT_EQ(quicklist->GetNthType(0), unity::QuicklistMenuItemType::LABEL);
  EXPECT_EQ(quicklist->GetNthType(1), unity::QuicklistMenuItemType::SEPARATOR);
  EXPECT_EQ(quicklist->GetNthType(2), unity::QuicklistMenuItemType::LABEL);
  EXPECT_EQ(quicklist->GetNthType(3), unity::QuicklistMenuItemType::CHECK);

  EXPECT_EQ(quicklist->GetNthItems(0)->GetLabel(), "label 0");
  EXPECT_EQ(quicklist->GetNthItems(2)->GetLabel(), "label 1");
  EXPECT_EQ(quicklist->GetNthItems(3)->GetLabel(), "check mark 0");
}

}
