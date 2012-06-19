// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <gtest/gtest.h>

#include "KeyboardUtil.h"

using namespace unity::ui;

namespace
{

TEST(TestKeyboardUtil, PrintableKeySymbols)
{
  EXPECT_TRUE(KeyboardUtil::IsPrintableKeySymbol(XK_Delete));
  EXPECT_TRUE(KeyboardUtil::IsPrintableKeySymbol(XK_BackSpace));
  EXPECT_TRUE(KeyboardUtil::IsPrintableKeySymbol(XK_space));
  EXPECT_TRUE(KeyboardUtil::IsPrintableKeySymbol(XK_3));
  EXPECT_TRUE(KeyboardUtil::IsPrintableKeySymbol(XK_v));
  EXPECT_TRUE(KeyboardUtil::IsPrintableKeySymbol(XK_1));
  EXPECT_TRUE(KeyboardUtil::IsPrintableKeySymbol(XK_ntilde));
  EXPECT_TRUE(KeyboardUtil::IsPrintableKeySymbol(XK_0));
  EXPECT_TRUE(KeyboardUtil::IsPrintableKeySymbol(XK_exclam));
  EXPECT_TRUE(KeyboardUtil::IsPrintableKeySymbol(XK_Home));
  EXPECT_TRUE(KeyboardUtil::IsPrintableKeySymbol(XK_End));

  EXPECT_FALSE(KeyboardUtil::IsPrintableKeySymbol(XK_F1));
  EXPECT_FALSE(KeyboardUtil::IsPrintableKeySymbol(XK_Select));
  EXPECT_FALSE(KeyboardUtil::IsPrintableKeySymbol(XK_Hyper_R));
  EXPECT_FALSE(KeyboardUtil::IsPrintableKeySymbol(XK_Control_L));
  EXPECT_FALSE(KeyboardUtil::IsPrintableKeySymbol(XK_Shift_L));
  EXPECT_FALSE(KeyboardUtil::IsPrintableKeySymbol(XK_Super_L));
  EXPECT_FALSE(KeyboardUtil::IsPrintableKeySymbol(XK_Print));
  EXPECT_FALSE(KeyboardUtil::IsPrintableKeySymbol(XK_Insert));
  EXPECT_FALSE(KeyboardUtil::IsPrintableKeySymbol(XK_Num_Lock));
  EXPECT_FALSE(KeyboardUtil::IsPrintableKeySymbol(XK_Caps_Lock));
}

} // Namespace
