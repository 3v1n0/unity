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

void test_key(Display* x_display, const char* key)
{
  KeySym keysym = XStringToKeysym(key);
  KeySym above_keysym = keyboard::get_key_above_key_symbol(x_display, keysym);
  EXPECT_NE(above_keysym, NoSymbol);
}

TEST(TestKeyboardUtil, AboveKeySymbol)
{
  Display* x_display = XOpenDisplay(NULL);

  test_key(x_display, "Tab");
  test_key(x_display, "Shift_R");
  test_key(x_display, "Control_L");
  test_key(x_display, "space");
  test_key(x_display, "comma");
  test_key(x_display, "a");
  test_key(x_display, "b");
  test_key(x_display, "c");
  test_key(x_display, "d");
  test_key(x_display, "e");
  test_key(x_display, "f");
  test_key(x_display, "g");
  test_key(x_display, "h");
  test_key(x_display, "i");
  test_key(x_display, "j");
  test_key(x_display, "k");
  test_key(x_display, "l");
  test_key(x_display, "m");
  test_key(x_display, "n");
  test_key(x_display, "o");
  test_key(x_display, "p");
  test_key(x_display, "k");
  test_key(x_display, "r");
  test_key(x_display, "s");
  test_key(x_display, "t");
  test_key(x_display, "u");
  test_key(x_display, "v");
  test_key(x_display, "w");
  test_key(x_display, "x");
  test_key(x_display, "y");
  test_key(x_display, "z");
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
