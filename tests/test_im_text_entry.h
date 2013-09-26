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

#ifndef TEST_IM_TEXT_ENTRY_H
#define TEST_IM_TEXT_ENTRY_H

#include <gmock/gmock.h>
#include "unity-shared/IMTextEntry.h"

struct TestEvent : nux::Event
{
  TestEvent(nux::KeyModifier keymod, unsigned long keysym);
  TestEvent(unsigned long keysym);
};

class MockTextEntry : public unity::IMTextEntry
{
public:
  MOCK_METHOD0(CutClipboard, void());
  MOCK_METHOD0(CopyClipboard, void());
  MOCK_METHOD0(PasteClipboard, void());
  MOCK_METHOD0(PastePrimaryClipboard, void());

  bool InspectKeyEvent(nux::Event const& event);
};

#endif // TEST_IM_TEXT_ENTRY_H
