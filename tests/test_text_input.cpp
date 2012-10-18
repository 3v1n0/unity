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
 * Authored by: Manuel de la Pena <manuel.delapena@canonical.com>
 *
 */

#include <gmock/gmock.h>
#include "unity-shared/TextInput.h"

using namespace testing;
using namespace unity;
using namespace nux;

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

class MockTextInput : public TextInput
{
public:
  MOCK_METHOD1(set_input_string, void(std::string const& string))
  MOCK_METHOD2(OnFontChanged, void(GtkSettings* settings,
              GParamSpec* pspec=NULL));
  MOCK_METHOD0(OnInputHintChanged, void());
  MOCK_METHOD4(OnMouseButtonDown, void(x, y, unsigned long button_flags,
              unsigned long key_flags));
  MOCK_METHOD0(OnEndKeyFocus, void());
};

TEST(TestTextInput, set_input_string)
{
}

TEST(TestTextInput, OnInputHintChanged)
{
}

TEST(TestTextInput, OnMouseButtonDown)
{
}

TEST(TestTextInput, OnEndKeyFocus)
{
}
