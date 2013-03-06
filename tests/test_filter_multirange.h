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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#ifndef TEST_FILTER
#define TEST_FILTER

#include <glib-object.h>
#include <gtest/gtest.h>

#include "UnityCore/MultiRangeFilter.h"

namespace unity
{
namespace testing
{

GVariantBuilder* AddFilterHint(GVariantBuilder* builder, const char* name, GVariant* value)
{
  if (builder == NULL)
    builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
  g_variant_builder_add (builder, "{sv}", name, value);
  return builder;
}

GVariant* AddFilterOptions(std::vector<bool> option_active)
{
  GVariantBuilder* builder;
  builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);

  int i = 0;
  for (auto iter = option_active.begin(), end = option_active.end(); iter != end; ++iter)
  {
    std::stringstream name;
    name << i++;

    const char* pstr_name = name.str().c_str();

    g_variant_builder_add (builder, "(sssb)", pstr_name, pstr_name, "", FALSE);
  }
  return g_variant_builder_end (builder);
}

void ExpectFilterRange(unity::dash::MultiRangeFilter::Ptr const& filter, int first, int last)
{
  int i = 0;
  for (auto option : filter->options())
  {
    bool should_be_active = i >= first && i <= last;
    EXPECT_EQ(option->active, should_be_active);
    i++;
  }
}

} // namespace testing
} // namespace unity

#endif