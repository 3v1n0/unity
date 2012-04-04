/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Daniel d'Andrada <daniel.dandrada@canonical.com>
 *
 */

#include <gtest/gtest.h>
#include <compiz_mock/core/core.h>
#include "GestureEngine.h"

CompScreenMock concrete_screen_mock;
CompScreenMock *screen_mock = &concrete_screen_mock;
int pointerX_mock = 0;
int pointerY_mock = 0;

TEST(GestureEngineTest, Foo)
{
  GestureEngine gestureEngine(screen_mock);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  int ret = RUN_ALL_TESTS();

  return ret;
}
