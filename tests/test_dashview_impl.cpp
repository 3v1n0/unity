/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Marco Biscaro <marcobiscaro2112@gmail.com>
 */

#include <gmock/gmock.h>

#include "DashViewPrivate.h"

using namespace testing;

using namespace unity::dash::impl;

namespace
{

TEST(TestParseLensFilter, TestSimpleString)
{
  LensFilter filter = parse_lens_uri("simple");

  EXPECT_THAT(filter.id, Eq("simple"));
  EXPECT_TRUE(filter.filters.empty());
}

TEST(TestParseLensFilter, TestNonFilterParameter)
{
  // Only params that start with "filter_" are added.
  LensFilter filter = parse_lens_uri("uri?param=test");

  EXPECT_THAT(filter.id, Eq("uri"));
  EXPECT_TRUE(filter.filters.empty());
}

TEST(TestParseLensFilter, TestSingleParameter)
{
  LensFilter filter = parse_lens_uri("uri?filter_param=test");

  EXPECT_THAT(filter.id, Eq("uri"));
  EXPECT_THAT(filter.filters.size(), Eq(1));
  EXPECT_THAT(filter.filters["param"], Eq("test"));
}

TEST(TestParseLensFilter, TestNoEquals)
{
  LensFilter filter = parse_lens_uri("uri?filter_param");

  EXPECT_THAT(filter.id, Eq("uri"));
  EXPECT_TRUE(filter.filters.empty());
}

TEST(TestParseLensFilter, TestEmbeddedEquals)
{
  LensFilter filter = parse_lens_uri("uri?filter_param=a=b");

  EXPECT_THAT(filter.id, Eq("uri"));
  EXPECT_THAT(filter.filters.size(), Eq(1));
  EXPECT_THAT(filter.filters["param"], Eq("a=b"));
}

TEST(TestParseLensFilter, TestMultipleParameters)
{
  LensFilter filter = parse_lens_uri("uri?filter_param1=first&filter_param2=second");

  EXPECT_THAT(filter.id, Eq("uri"));
  EXPECT_THAT(filter.filters.size(), Eq(2));
  EXPECT_THAT(filter.filters["param1"], Eq("first"));
  EXPECT_THAT(filter.filters["param2"], Eq("second"));
}

}
