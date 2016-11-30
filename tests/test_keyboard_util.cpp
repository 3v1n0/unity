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
#include <algorithm>

#include <X11/keysym.h>

#include "KeyboardUtil.h"
#include "XKeyboardUtil.h"

using namespace unity;

namespace
{

TEST(TestKeyboardUtil, AboveKeySymbol)
{
  Display* x_display = XOpenDisplay(NULL);

  ASSERT_EQ(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("escape")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("Tab")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("Shift_R")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("Control_L")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("space")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("comma")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("a")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("b")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("c")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("d")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("e")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("f")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("g")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("h")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("i")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("j")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("k")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("l")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("m")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("n")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("o")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("p")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("k")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("r")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("s")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("t")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("u")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("v")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("w")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("x")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("y")), NoSymbol);
  ASSERT_NE(keyboard::get_key_above_key_symbol(x_display, XStringToKeysym("z")), NoSymbol);

  XCloseDisplay(x_display);
}

TEST(TestKeyboardUtil, BelowKeySymbol)
{
  Display* x_display = XOpenDisplay(NULL);

  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("Tab")), NoSymbol);
  ASSERT_EQ(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("Control_L")), NoSymbol);
  ASSERT_EQ(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("space")), NoSymbol);
  ASSERT_EQ(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("comma")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("a")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("b")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("d")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("e")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("f")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("g")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("h")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("i")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("j")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("k")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("l")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("o")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("p")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("k")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("r")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("s")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("t")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("u")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("w")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("x")), NoSymbol);
  ASSERT_NE(keyboard::get_key_below_key_symbol(x_display, XStringToKeysym("y")), NoSymbol);

  XCloseDisplay(x_display);
}

TEST(TestKeyboardUtil, RightToKeySymbol)
{
  Display* x_display = XOpenDisplay(NULL);

  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("Tab")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("Shift_R")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("Control_L")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("space")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("comma")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("a")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("b")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("c")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("d")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("e")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("f")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("g")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("h")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("i")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("j")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("k")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("l")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("m")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("n")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("o")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("p")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("k")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("r")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("s")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("t")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("u")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("v")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("w")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("x")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("y")), NoSymbol);
  ASSERT_NE(keyboard::get_key_right_to_key_symbol(x_display, XStringToKeysym("z")), NoSymbol);

  XCloseDisplay(x_display);
}

TEST(TestKeyboardUtil, LeftToKeySymbol)
{
  Display* x_display = XOpenDisplay(NULL);

  ASSERT_EQ(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("Tab")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("Shift_R")), NoSymbol);
  ASSERT_EQ(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("Control_L")), NoSymbol);
  ASSERT_EQ(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("escape")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("space")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("comma")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("a")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("b")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("c")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("d")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("e")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("f")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("g")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("h")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("i")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("j")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("k")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("l")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("m")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("n")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("o")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("p")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("k")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("r")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("s")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("t")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("u")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("v")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("w")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("x")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("y")), NoSymbol);
  ASSERT_NE(keyboard::get_key_left_to_key_symbol(x_display, XStringToKeysym("z")), NoSymbol);

  XCloseDisplay(x_display);
}

TEST(TestKeyboardUtil, PrintableKeySymbols)
{
  EXPECT_TRUE(keyboard::is_printable_key_symbol(XK_Delete));
  EXPECT_TRUE(keyboard::is_printable_key_symbol(XK_BackSpace));
  EXPECT_TRUE(keyboard::is_printable_key_symbol(XK_space));
  EXPECT_TRUE(keyboard::is_printable_key_symbol(XK_3));
  EXPECT_TRUE(keyboard::is_printable_key_symbol(XK_v));
  EXPECT_TRUE(keyboard::is_printable_key_symbol(XK_1));
  EXPECT_TRUE(keyboard::is_printable_key_symbol(XK_ntilde));
  EXPECT_TRUE(keyboard::is_printable_key_symbol(XK_0));
  EXPECT_TRUE(keyboard::is_printable_key_symbol(XK_exclam));

  EXPECT_FALSE(keyboard::is_printable_key_symbol(XK_F1));
  EXPECT_FALSE(keyboard::is_printable_key_symbol(XK_Select));
  EXPECT_FALSE(keyboard::is_printable_key_symbol(XK_Hyper_R));
  EXPECT_FALSE(keyboard::is_printable_key_symbol(XK_Control_L));
  EXPECT_FALSE(keyboard::is_printable_key_symbol(XK_Shift_L));
  EXPECT_FALSE(keyboard::is_printable_key_symbol(XK_Super_L));
  EXPECT_FALSE(keyboard::is_printable_key_symbol(XK_Print));
  EXPECT_FALSE(keyboard::is_printable_key_symbol(XK_Insert));
  EXPECT_FALSE(keyboard::is_printable_key_symbol(XK_Num_Lock));
  EXPECT_FALSE(keyboard::is_printable_key_symbol(XK_Caps_Lock));
  EXPECT_FALSE(keyboard::is_printable_key_symbol(XK_ISO_Level3_Shift));
}

TEST(TestKeyboardUtil, MoveKeySymbols)
{
  std::vector<KeySym> move_symbols { XK_Home, XK_Left, XK_Up, XK_Right, XK_Down,
                                     XK_Prior, XK_Page_Up, XK_Next, XK_Page_Down,
                                     XK_End, XK_Begin };

  for (KeySym sym = 0; sym < XK_VoidSymbol; ++sym)
  {
    if (std::find(move_symbols.begin(), move_symbols.end(), sym) != move_symbols.end())
      EXPECT_TRUE(keyboard::is_move_key_symbol(sym));
    else
      EXPECT_FALSE(keyboard::is_move_key_symbol(sym));
  }
}

} // Namespace
