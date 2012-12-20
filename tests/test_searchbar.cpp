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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unity-shared/SearchBarSpinner.h>
#include <unity-shared/DashStyle.h>
#include <unity-shared/UnitySettings.h>

using namespace unity;

namespace unity
{

namespace
{
gboolean TimeoutReached (gpointer data)
{
  bool *b = static_cast<bool*>(data);

  *b = true;

  return FALSE;
}
}

class TestSearchBar : public ::testing::Test
{
public:
  TestSearchBar() {}

  unity::Settings settings;
  dash::Style style;
};

TEST_F(TestSearchBar, TestSearchBarSpinnerTimeout)
{
  SearchBarSpinner search_bar;
  search_bar.SetSpinnerTimeout(50);
  search_bar.SetState(STATE_SEARCHING);

  ASSERT_TRUE(search_bar.GetState() == STATE_SEARCHING);

  bool done = false;
  volatile bool timeout_reached = false;
  guint tid = g_timeout_add (100, TimeoutReached, (gpointer)(&timeout_reached));
  while (!timeout_reached && !done)
  {
    g_main_context_iteration (NULL, TRUE);
    done = search_bar.GetState() == STATE_READY;
  }

  ASSERT_TRUE(done);

  g_source_remove (tid);
}

TEST_F(TestSearchBar, TestSearchBarSpinnerNoTimeout)
{
  SearchBarSpinner search_bar;
  search_bar.SetSpinnerTimeout(-1);
  search_bar.SetState(STATE_SEARCHING);

  ASSERT_TRUE(search_bar.GetState() == STATE_SEARCHING);

  bool done = false;
  volatile bool timeout_reached = false;
  guint tid = g_timeout_add (200, TimeoutReached, (gpointer)(&timeout_reached));
  while (!timeout_reached && !done)
  {
    g_main_context_iteration (NULL, TRUE);
    done = search_bar.GetState() == STATE_READY;
  }

  ASSERT_TRUE(!done);

  g_source_remove (tid);
}

}
