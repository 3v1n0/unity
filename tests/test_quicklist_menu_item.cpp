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

#include "QuicklistMenuItem.h"
#include "QuicklistMenuItemCheckmark.h"
#include "QuicklistMenuItemLabel.h"
#include "QuicklistMenuItemRadio.h"
#include "QuicklistMenuItemSeparator.h"

using namespace unity;
using namespace testing;

namespace
{

struct TestQuicklistMenuItem : public Test
{
  TestQuicklistMenuItem()
    : item(dbusmenu_menuitem_new())
  {}

  glib::Object<DbusmenuMenuitem> item;
};

TEST_F(TestQuicklistMenuItem, QuicklistMenuItemCheckmark)
{
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, "Unchecked");
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE,
                                       DBUSMENU_MENUITEM_TOGGLE_CHECK);
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, false);
  dbusmenu_menuitem_property_set_int(item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
                                           DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);

  nux::ObjectPtr<QuicklistMenuItemCheckmark> qlitem(new QuicklistMenuItemCheckmark(item));

  EXPECT_EQ(qlitem->GetLabel(), "Unchecked");
  EXPECT_FALSE(qlitem->GetEnabled());
  EXPECT_FALSE(qlitem->GetActive());
  EXPECT_FALSE(qlitem->GetSelectable());
  EXPECT_FALSE(qlitem->IsMarkupEnabled());
}

TEST_F(TestQuicklistMenuItem, QuicklistMenuItemLabel)
{
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, "A Label");
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_property_set_bool(item, "unity-use-markup", true);

  nux::ObjectPtr<QuicklistMenuItemLabel> qlitem(new QuicklistMenuItemLabel(item));

  EXPECT_EQ(qlitem->GetLabel(), "A Label");
  EXPECT_TRUE(qlitem->GetEnabled());
  EXPECT_TRUE(qlitem->GetSelectable());
  EXPECT_TRUE(qlitem->IsMarkupEnabled());
}

TEST_F(TestQuicklistMenuItem, QuicklistMenuItemRadio)
{
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, "Radio Active");
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE,
                                       DBUSMENU_MENUITEM_TOGGLE_RADIO);
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_property_set_int(item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
                                           DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED);

  nux::ObjectPtr<QuicklistMenuItemRadio> qlitem(new QuicklistMenuItemRadio(item));
  qlitem->EnableLabelMarkup(true);

  EXPECT_EQ(qlitem->GetLabel(), "Radio Active");
  EXPECT_TRUE(qlitem->GetEnabled());
  EXPECT_TRUE(qlitem->GetActive());
  EXPECT_TRUE(qlitem->GetSelectable());
  EXPECT_TRUE(qlitem->IsMarkupEnabled());
}

TEST_F(TestQuicklistMenuItem, QuicklistMenuItemSeparator)
{
  dbusmenu_menuitem_property_set(item, "type", DBUSMENU_CLIENT_TYPES_SEPARATOR);
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, true);

  nux::ObjectPtr<QuicklistMenuItemSeparator> qlitem(new QuicklistMenuItemSeparator(item));

  EXPECT_TRUE(qlitem->GetEnabled());
  EXPECT_FALSE(qlitem->GetSelectable());
}

}
