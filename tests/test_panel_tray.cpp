/*
 * Copyright 2013 Canonical Ltd.
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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 *
 */

#include <gtest/gtest.h>

#include "panel/PanelTray.h"

 TEST(TestPanelTray, FilterTray)
 {
  EXPECT_TRUE(unity::PanelTray::FilterTray("JavaEmbeddedFrame", "", ""));
  EXPECT_TRUE(unity::PanelTray::FilterTray("", "JavaEmbeddedFrame", ""));
  EXPECT_TRUE(unity::PanelTray::FilterTray("", "", "JavaEmbeddedFrame"));

  EXPECT_TRUE(unity::PanelTray::FilterTray("Wine", "", ""));
  EXPECT_TRUE(unity::PanelTray::FilterTray("", "Wine", ""));
  EXPECT_TRUE(unity::PanelTray::FilterTray("", "", "Wine"));

  EXPECT_TRUE(unity::PanelTray::FilterTray("JavaEmbeddedFrameUbuntu", "", ""));
  EXPECT_TRUE(unity::PanelTray::FilterTray("", "JavaEmbeddedFrameUbuntu", ""));
  EXPECT_TRUE(unity::PanelTray::FilterTray("", "", "JavaEmbeddedFrameUbuntu"));

  EXPECT_TRUE(unity::PanelTray::FilterTray("WineUbuntu", "", ""));
  EXPECT_TRUE(unity::PanelTray::FilterTray("", "WineUbuntu", ""));
  EXPECT_TRUE(unity::PanelTray::FilterTray("", "", "WineUbuntu"));

  EXPECT_FALSE(unity::PanelTray::FilterTray("UbuntuJavaEmbeddedFrame", "", ""));
  EXPECT_FALSE(unity::PanelTray::FilterTray("", "UbuntuJavaEmbeddedFrame", ""));
  EXPECT_FALSE(unity::PanelTray::FilterTray("", "", "UbuntuJavaEmbeddedFrame"));

  EXPECT_FALSE(unity::PanelTray::FilterTray("UbuntuWine", "", ""));
  EXPECT_FALSE(unity::PanelTray::FilterTray("", "UbuntuWine", ""));
  EXPECT_FALSE(unity::PanelTray::FilterTray("", "", "UbuntuWine"));

  EXPECT_FALSE(unity::PanelTray::FilterTray("UbuntuJavaEmbeddedFrameUbuntu", "", ""));
  EXPECT_FALSE(unity::PanelTray::FilterTray("", "UbuntuJavaEmbeddedFrameUbuntu", ""));
  EXPECT_FALSE(unity::PanelTray::FilterTray("", "", "UbuntuJavaEmbeddedFrameUbuntu"));

  EXPECT_FALSE(unity::PanelTray::FilterTray("UbuntuWineUbuntu", "", ""));
  EXPECT_FALSE(unity::PanelTray::FilterTray("", "UbuntuWineUbuntu", ""));
  EXPECT_FALSE(unity::PanelTray::FilterTray("", "", "UbuntuWineUbuntu"));

  EXPECT_TRUE(unity::PanelTray::FilterTray("Wine", "Ubuntu", ""));
  EXPECT_TRUE(unity::PanelTray::FilterTray("Ubuntu", "JavaEmbeddedFrame", ""));
  EXPECT_TRUE(unity::PanelTray::FilterTray("Wine", "JavaEmbeddedFrame", "Ubuntu"));

  EXPECT_FALSE(unity::PanelTray::FilterTray("Ubuntu", "Unity", "Hello world!"));
}

