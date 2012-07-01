/*
 * Copyright 2012 Canonical Ltd.
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

#include <gmock/gmock.h>
#include "unity-shared/IMTextEntry.h"

using namespace testing;
using namespace unity;

namespace
{

class TestEvent : public nux::Event
{
public:
  TestEvent(nux::KeyModifier keymod, unsigned long keysym)
  {
    type = NUX_KEYDOWN;
    key_modifiers = keymod;
    x11_keysym = keysym;
  }

  TestEvent(unsigned long keysym)
  {
    type = NUX_KEYDOWN;
    x11_keysym = keysym;
  }
};

class MockTextEntry : public IMTextEntry
{
public:
  MOCK_METHOD1(InsertText, void(std::string const&));
  MOCK_METHOD0(Cut, void());
  MOCK_METHOD0(Copy, void());
  MOCK_METHOD1(Paste, void(bool));

  bool TryHandleSpecial(nux::Event const& event)
  {
    return IMTextEntry::TryHandleSpecial(event);
  }
};


TEST(TestIMTextEntry, CopyCtrlC)
{
  MockTextEntry text_entry;

  TestEvent event(KEY_MODIFIER_CTRL, NUX_VK_c);

  EXPECT_CALL(text_entry, Copy());
  EXPECT_FALSE(text_entry.TryHandleSpecial(event));
}

TEST(TestIMTextEntry, CopyCtrlIns)
{
  MockTextEntry text_entry;

  TestEvent event(KEY_MODIFIER_CTRL, NUX_VK_INSERT);

  EXPECT_CALL(text_entry, Copy());
  EXPECT_FALSE(text_entry.TryHandleSpecial(event));
}

TEST(TestIMTextEntry, PasteCtrlV)
{
  MockTextEntry text_entry;

  TestEvent event(KEY_MODIFIER_CTRL, NUX_VK_v);

  EXPECT_CALL(text_entry, Paste(false));
  EXPECT_FALSE(text_entry.TryHandleSpecial(event));
}

TEST(TestIMTextEntry, PasteShiftIns)
{
  MockTextEntry text_entry;

  TestEvent event(KEY_MODIFIER_SHIFT, NUX_VK_INSERT);

  EXPECT_CALL(text_entry, Paste(false));
  EXPECT_FALSE(text_entry.TryHandleSpecial(event));
}

TEST(TestIMTextEntry, CutCtrlX)
{
  MockTextEntry text_entry;

  TestEvent event(KEY_MODIFIER_CTRL, NUX_VK_x);

  EXPECT_CALL(text_entry, Cut());
  EXPECT_FALSE(text_entry.TryHandleSpecial(event));
}

TEST(TestIMTextEntry, CutShiftDel)
{
  MockTextEntry text_entry;

  TestEvent event(KEY_MODIFIER_SHIFT, NUX_VK_DELETE);

  EXPECT_CALL(text_entry, Cut());
  EXPECT_FALSE(text_entry.TryHandleSpecial(event));
}

TEST(TestIMTextEntry, CtrlMoveKeys)
{
  MockTextEntry text_entry;

  TestEvent left(KEY_MODIFIER_CTRL, NUX_VK_LEFT);
  EXPECT_TRUE(text_entry.TryHandleSpecial(left));

  TestEvent right(KEY_MODIFIER_CTRL, NUX_VK_RIGHT);
  EXPECT_TRUE(text_entry.TryHandleSpecial(right));

  TestEvent home(KEY_MODIFIER_CTRL, NUX_VK_HOME);
  EXPECT_TRUE(text_entry.TryHandleSpecial(home));

  TestEvent end(KEY_MODIFIER_CTRL, NUX_VK_END);
  EXPECT_TRUE(text_entry.TryHandleSpecial(end));
}

TEST(TestIMTextEntry, CtrlDeleteKeys)
{
  MockTextEntry text_entry;

  TestEvent del(KEY_MODIFIER_CTRL, NUX_VK_DELETE);
  EXPECT_TRUE(text_entry.TryHandleSpecial(del));

  TestEvent backspace(KEY_MODIFIER_CTRL, NUX_VK_BACKSPACE);
  EXPECT_TRUE(text_entry.TryHandleSpecial(backspace));
}

TEST(TestIMTextEntry, CtrlA)
{
  MockTextEntry text_entry;

  TestEvent selectall(KEY_MODIFIER_CTRL, NUX_VK_a);
  EXPECT_TRUE(text_entry.TryHandleSpecial(selectall));
}

TEST(TestIMTextEntry, CtrlKeybindings)
{
  MockTextEntry text_entry;

  std::vector<unsigned long> allowed_keys { NUX_VK_a, NUX_VK_BACKSPACE,
                                            NUX_VK_LEFT, NUX_VK_RIGHT,
                                            NUX_VK_HOME, NUX_VK_END,
                                            NUX_VK_BACKSPACE, NUX_VK_DELETE };

  for (unsigned long keysym = 0; keysym < XK_VoidSymbol; ++keysym)
  {
    bool should_be_handled = false;

    if (std::find(allowed_keys.begin(), allowed_keys.end(), keysym) != allowed_keys.end())
      should_be_handled = true;

    TestEvent event(KEY_MODIFIER_CTRL, keysym);
    EXPECT_EQ(text_entry.TryHandleSpecial(event), should_be_handled);
  }
}

TEST(TestIMTextEntry, AltKeybindings)
{
  MockTextEntry text_entry;

  for (unsigned long keysym = 0; keysym < XK_VoidSymbol; ++keysym)
  {
    TestEvent event(KEY_MODIFIER_ALT, keysym);
    EXPECT_FALSE(text_entry.TryHandleSpecial(event));
  }
}

TEST(TestIMTextEntry, SuperKeybindings)
{
  MockTextEntry text_entry;

  for (unsigned long keysym = 0; keysym < XK_VoidSymbol; ++keysym)
  {
    TestEvent event(KEY_MODIFIER_SUPER, keysym);
    EXPECT_FALSE(text_entry.TryHandleSpecial(event));
  }
}

}
