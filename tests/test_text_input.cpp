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


#include <gtest/gtest.h>

#include "unity-shared/TextInput.h"
#include "test_utils.h"

using namespace nux;

namespace unity
{

class TestTextInput : public ::testing::Test
{
   protected:
     TestTextInput()
     {
       entry = new TextInput();
     }

     TextInput* entry;
};

TEST_F(TestTextInput, HintCorrectInit)
{
}

TEST_F(TestTextInput, EntryCorrectInit)
{
}

TEST_F(TestTextInput, InputStringCorrectSetter)
{
}

TEST_F(TestTextInput, OnFontChanged)
{
}

TEST_F(TestTextInput, OnInputHintChanged)
{
}

TEST_F(TestTextInput, OnMouseButtonDown)
{
}

TEST_F(TestTextInput, OnEndKeyFocus)
{
}

} // unity
