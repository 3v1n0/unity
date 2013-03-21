/*
 * Copyright 2012-2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 *
 */

#include "test_im_text_entry.h"

using namespace nux;

TEST(TestIMTextEntry, AltKeybindings)
{
  MockTextEntry text_entry;

  for (unsigned long keysym = 0; keysym < XK_VoidSymbol; ++keysym)
  {
    TestEvent event(KEY_MODIFIER_ALT, keysym);
    EXPECT_FALSE(text_entry.InspectKeyEvent(event));
  }
}

TEST(TestIMTextEntry, SuperKeybindings)
{
  MockTextEntry text_entry;

  for (unsigned long keysym = 0; keysym < XK_VoidSymbol; ++keysym)
  {
    TestEvent event(KEY_MODIFIER_SUPER, keysym);
    EXPECT_FALSE(text_entry.InspectKeyEvent(event));
  }
}
