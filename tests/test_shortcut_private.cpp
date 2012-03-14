// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 */

#include <gtest/gtest.h>

#include "ShortcutHintPrivate.h"

using namespace unity::shortcut::impl;

namespace {

TEST(TestShortcutHintPrivate, TestFixShortcutFormatCorrect)
{
  EXPECT_EQ(FixShortcutFormat("Super"), "Super");
  EXPECT_EQ(FixShortcutFormat("Super + A"), "Super + A");
}


TEST(TestShortcutHintPrivate, TestFixShortcutFormatLT)
{
  EXPECT_EQ(FixShortcutFormat("<Super"), "Super");
  EXPECT_EQ(FixShortcutFormat("<Super + <Alt"), "Super + Alt");
}

TEST(TestShortcutHintPrivate, TestFixShortcutFormatGT)
{
  EXPECT_EQ(FixShortcutFormat("Super>"), "Super");
  EXPECT_EQ(FixShortcutFormat("Super>A"), "Super + A");
  EXPECT_EQ(FixShortcutFormat("Super>Alt>A"), "Super + Alt + A");
}

TEST(TestShortcutHintPrivate, TestFixShortcutComplete)
{
  EXPECT_EQ(FixShortcutFormat("Super"), "Super");
  EXPECT_EQ(FixShortcutFormat("Super + A"), "Super + A");
  EXPECT_EQ(FixShortcutFormat("<Super>"), "Super");
  EXPECT_EQ(FixShortcutFormat("<Super>A"), "Super + A");
  EXPECT_EQ(FixShortcutFormat("<Super><Alt>A"), "Super + Alt + A");
}

TEST(TestShortcutHintPrivate, TestProperCase)
{
  EXPECT_EQ(ProperCase("super"), "Super");
  EXPECT_EQ(ProperCase("sUper"), "SUper");
  EXPECT_EQ(ProperCase("super + a"), "Super + A");
  EXPECT_EQ(ProperCase("super+a"), "Super+A");
  EXPECT_EQ(ProperCase("<super><alt>a"), "<Super><Alt>A");
}

TEST(TestShortcutHintPrivate, TestFixMouseShortcut)
{
  EXPECT_EQ(FixMouseShortcut("Super<Button1>"), "Super<Left Mouse>");
  EXPECT_EQ(FixMouseShortcut("Super<Button2>"), "Super<Middle Mouse>");
  EXPECT_EQ(FixMouseShortcut("Super<Button3>"), "Super<Right Mouse>");
}

TEST(TestShortcutHintPrivate, TestGetMetaKey)
{
  EXPECT_EQ(GetMetaKey("<Super>"), "<Super>");
  EXPECT_EQ(GetMetaKey("<Super><Alt>"), "<Super><Alt>");
  EXPECT_EQ(GetMetaKey("<Super>A"), "<Super>");
  EXPECT_EQ(GetMetaKey("<Super><Alt>A"), "<Super><Alt>");
  EXPECT_EQ(GetMetaKey("ABC"), "");
}

} // anonymouse namespace
