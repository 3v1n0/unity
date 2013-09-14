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

using namespace testing;
using namespace unity;
using namespace nux;

TEST(TestIMTextEntry, CopyCtrlC)
{
  MockTextEntry text_entry;

  TestEvent event(KEY_MODIFIER_CTRL, NUX_VK_c);

  EXPECT_CALL(text_entry, CopyClipboard());
  EXPECT_TRUE(text_entry.InspectKeyEvent(event));
}

TEST(TestIMTextEntry, CopyCtrlIns)
{
  MockTextEntry text_entry;

  TestEvent event(KEY_MODIFIER_CTRL, NUX_VK_INSERT);

  EXPECT_CALL(text_entry, CopyClipboard());
  EXPECT_TRUE(text_entry.InspectKeyEvent(event));
}

TEST(TestIMTextEntry, PasteCtrlV)
{
  MockTextEntry text_entry;

  TestEvent event(KEY_MODIFIER_CTRL, NUX_VK_v);

  EXPECT_CALL(text_entry, PasteClipboard());
  EXPECT_TRUE(text_entry.InspectKeyEvent(event));
}

TEST(TestIMTextEntry, PasteShiftIns)
{
  MockTextEntry text_entry;

  TestEvent event(KEY_MODIFIER_SHIFT, NUX_VK_INSERT);

  EXPECT_CALL(text_entry, PasteClipboard());
  EXPECT_TRUE(text_entry.InspectKeyEvent(event));
}

TEST(TestIMTextEntry, CutCtrlX)
{
  MockTextEntry text_entry;

  TestEvent event(KEY_MODIFIER_CTRL, NUX_VK_x);

  EXPECT_CALL(text_entry, CutClipboard());
  EXPECT_TRUE(text_entry.InspectKeyEvent(event));
}

TEST(TestIMTextEntry, CutShiftDel)
{
  MockTextEntry text_entry;

  TestEvent event(KEY_MODIFIER_SHIFT, NUX_VK_DELETE);

  EXPECT_CALL(text_entry, CutClipboard());
  EXPECT_TRUE(text_entry.InspectKeyEvent(event));
}

TEST(TestIMTextEntry, CtrlMoveKeys)
{
  MockTextEntry text_entry;

  TestEvent left(KEY_MODIFIER_CTRL, NUX_VK_LEFT);
  EXPECT_TRUE(text_entry.InspectKeyEvent(left));

  TestEvent right(KEY_MODIFIER_CTRL, NUX_VK_RIGHT);
  EXPECT_TRUE(text_entry.InspectKeyEvent(right));

  TestEvent home(KEY_MODIFIER_CTRL, NUX_VK_HOME);
  EXPECT_TRUE(text_entry.InspectKeyEvent(home));

  TestEvent end(KEY_MODIFIER_CTRL, NUX_VK_END);
  EXPECT_TRUE(text_entry.InspectKeyEvent(end));
}

TEST(TestIMTextEntry, CtrlDeleteKeys)
{
  MockTextEntry text_entry;

  TestEvent del(KEY_MODIFIER_CTRL, NUX_VK_DELETE);
  EXPECT_TRUE(text_entry.InspectKeyEvent(del));

  TestEvent backspace(KEY_MODIFIER_CTRL, NUX_VK_BACKSPACE);
  EXPECT_TRUE(text_entry.InspectKeyEvent(backspace));
}

TEST(TestIMTextEntry, CtrlA)
{
  MockTextEntry text_entry;

  TestEvent selectall(KEY_MODIFIER_CTRL, NUX_VK_a);
  EXPECT_TRUE(text_entry.InspectKeyEvent(selectall));
}

TEST(TestIMTextEntry, CtrlKeybindings)
{
  MockTextEntry text_entry;

  std::vector<unsigned long> allowed_keys { NUX_VK_a, NUX_VK_BACKSPACE,
                                            NUX_VK_LEFT, NUX_VK_RIGHT,
                                            NUX_VK_HOME, NUX_VK_END,
                                            NUX_VK_BACKSPACE, NUX_VK_DELETE };

  for (auto keysym : allowed_keys)
  {
    TestEvent event(KEY_MODIFIER_CTRL, keysym);
    EXPECT_TRUE(text_entry.InspectKeyEvent(event));
  }
}
