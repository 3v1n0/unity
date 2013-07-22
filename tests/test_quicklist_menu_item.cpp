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
#include <UnityCore/GLibSignal.h>

#include "QuicklistMenuItem.h"
#include "QuicklistMenuItemCheckmark.h"
#include "QuicklistMenuItemLabel.h"
#include "QuicklistMenuItemRadio.h"
#include "QuicklistMenuItemSeparator.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/UBusMessages.h"
#include "test_utils.h"

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
  dbusmenu_menuitem_property_set_bool(item, QuicklistMenuItem::MARKUP_ENABLED_PROPERTY, true);

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

TEST_F(TestQuicklistMenuItem, OverlayMenuitem)
{
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, "Label");
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, true);

  nux::ObjectPtr<QuicklistMenuItemLabel> qlitem(new QuicklistMenuItemLabel(item));

  EXPECT_FALSE(qlitem->IsOverlayQuicklist());

  dbusmenu_menuitem_property_set_bool(item, QuicklistMenuItem::OVERLAY_MENU_ITEM_PROPERTY, true);
  EXPECT_TRUE(qlitem->IsOverlayQuicklist());
}

TEST_F(TestQuicklistMenuItem, MaxLabelWidth)
{
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, "Label");
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, true);

  nux::ObjectPtr<QuicklistMenuItemLabel> qlitem(new QuicklistMenuItemLabel(item));
  int max_width = 200;

  EXPECT_EQ(qlitem->GetMaxLabelWidth(), 0);

  dbusmenu_menuitem_property_set_int(item, QuicklistMenuItem::MAXIMUM_LABEL_WIDTH_PROPERTY, max_width);
  EXPECT_EQ(qlitem->GetMaxLabelWidth(), max_width);

  max_width = 100;
  qlitem->SetMaxLabelWidth(max_width);
  EXPECT_EQ(dbusmenu_menuitem_property_get_int(item, QuicklistMenuItem::MAXIMUM_LABEL_WIDTH_PROPERTY), max_width);
}

TEST_F(TestQuicklistMenuItem, MarkupAccelEnabled)
{
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, "Label");
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, true);

  nux::ObjectPtr<QuicklistMenuItemLabel> qlitem(new QuicklistMenuItemLabel(item));
  EXPECT_TRUE(qlitem->IsMarkupAccelEnabled());

  dbusmenu_menuitem_property_set_bool(item, QuicklistMenuItem::MARKUP_ACCEL_DISABLED_PROPERTY, true);
  EXPECT_FALSE(qlitem->IsMarkupAccelEnabled());

  qlitem->EnableLabelMarkupAccel(true);
  EXPECT_TRUE(qlitem->IsMarkupAccelEnabled());
  EXPECT_FALSE(dbusmenu_menuitem_property_get_bool(item, QuicklistMenuItem::MARKUP_ACCEL_DISABLED_PROPERTY));

  qlitem->EnableLabelMarkupAccel(false);
  EXPECT_FALSE(qlitem->IsMarkupAccelEnabled());
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(item, QuicklistMenuItem::MARKUP_ACCEL_DISABLED_PROPERTY));
}

TEST_F(TestQuicklistMenuItem, ItemActivate)
{
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, "Label");
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, true);

  XEvent xevent;
  xevent.type = ButtonPress;
  xevent.xany.display = nux::GetGraphicsDisplay()->GetX11Display();
  xevent.xany.window = nux::GetGraphicsDisplay()->GetWindowHandle();
  xevent.xbutton.time = g_random_int();
  xevent.xbutton.button = Button1;
  nux::GetGraphicsDisplay()->ProcessXEvent(xevent, true);

  auto event_time = nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp;
  ASSERT_EQ(xevent.xbutton.time, event_time);

  nux::ObjectPtr<QuicklistMenuItemLabel> qlitem(new QuicklistMenuItemLabel(item));

  bool item_activated = false;
  glib::Signal<void, DbusmenuMenuitem*, unsigned> signal(item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
  [this, event_time, &item_activated] (DbusmenuMenuitem* menu_item, unsigned time) {
    EXPECT_EQ(menu_item, item);
    EXPECT_EQ(time, event_time);
    item_activated = true;
  });

  qlitem->Activate();
  EXPECT_TRUE(item_activated);
}

TEST_F(TestQuicklistMenuItem, ItemActivateClosesDash)
{
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, "Label");
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, true);

  nux::ObjectPtr<QuicklistMenuItemLabel> qlitem(new QuicklistMenuItemLabel(item));

  bool closes_dash = false;
  UBusManager manager;
  manager.RegisterInterest(UBUS_OVERLAY_CLOSE_REQUEST, [&] (GVariant*) { closes_dash = true; });

  qlitem->Activate();
  Utils::WaitUntil(closes_dash);

  EXPECT_TRUE(closes_dash);
}

TEST_F(TestQuicklistMenuItem, OverlayItemActivateDoesNotCloseDash)
{
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, "Label");
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_property_set_bool(item, QuicklistMenuItem::OVERLAY_MENU_ITEM_PROPERTY, true);

  nux::ObjectPtr<QuicklistMenuItemLabel> qlitem(new QuicklistMenuItemLabel(item));

  bool closes_dash = false;
  UBusManager manager;
  manager.RegisterInterest(UBUS_OVERLAY_CLOSE_REQUEST, [&] (GVariant*) { closes_dash = true; });

  qlitem->Activate();
  Utils::WaitForTimeoutMSec(100);

  EXPECT_FALSE(closes_dash);
}

}
