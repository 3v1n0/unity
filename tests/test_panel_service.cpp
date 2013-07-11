// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Variant.h>
#include "panel-service.h"
#include "mock_indicator_object.h"

using namespace testing;
using namespace unity;

namespace
{
const std::string SYNC_ENTRY_VARIANT_FORMAT = "(ssssbbusbbi)";
const std::string SYNC_ENTRIES_VARIANT_FORMAT = "(a"+SYNC_ENTRY_VARIANT_FORMAT+")";

struct TestPanelService : Test
{
  TestPanelService()
    : service(panel_service_get_default_with_indicators(nullptr))
  {}

  std::size_t GetIndicatorsInResult(glib::Variant const& result) const
  {
    EXPECT_TRUE(result);
    if (!result)
      return 0;

    std::set<std::string> objects;
    GVariantIter* iter;
    gchar* indicator_id;
    gchar* entry_id;
    gchar* entry_name_hint;
    gchar* label;
    gboolean label_sensitive;
    gboolean label_visible;
    guint32 image_type;
    gchar* image_data;
    gboolean image_sensitive;
    gboolean image_visible;
    gboolean priority;

    g_variant_get(result, SYNC_ENTRIES_VARIANT_FORMAT.c_str(), &iter);
    while (g_variant_iter_loop(iter, SYNC_ENTRY_VARIANT_FORMAT.c_str(),
                               &indicator_id,
                               &entry_id,
                               &entry_name_hint,
                               &label,
                               &label_sensitive,
                               &label_visible,
                               &image_type,
                               &image_data,
                               &image_sensitive,
                               &image_visible,
                               &priority))
    {
      auto const& id = glib::gchar_to_string(indicator_id);

      if (!id.empty())
        objects.insert(id);
    }
    g_variant_iter_free(iter);

    return objects.size();
  }

  std::size_t GetEntriesInResult(glib::Variant const& result) const
  {
    EXPECT_TRUE(result);
    if (!result)
      return 0;

    std::set<std::string> entries;
    GVariantIter* iter;
    gchar* indicator_id;
    gchar* entry_id;
    gchar* entry_name_hint;
    gchar* label;
    gboolean label_sensitive;
    gboolean label_visible;
    guint32 image_type;
    gchar* image_data;
    gboolean image_sensitive;
    gboolean image_visible;
    gboolean priority;

    g_variant_get(result, SYNC_ENTRIES_VARIANT_FORMAT.c_str(), &iter);
    while (g_variant_iter_loop(iter, SYNC_ENTRY_VARIANT_FORMAT.c_str(),
                               &indicator_id,
                               &entry_id,
                               &entry_name_hint,
                               &label,
                               &label_sensitive,
                               &label_visible,
                               &image_type,
                               &image_data,
                               &image_sensitive,
                               &image_visible,
                               &priority))
    {
      auto const& entry = glib::gchar_to_string(entry_id);

      if (!entry.empty())
        entries.insert(entry);
    }
    g_variant_iter_free(iter);

    return entries.size();
  }

  glib::Object<PanelService> service;
};

TEST_F(TestPanelService, Construction)
{
  ASSERT_TRUE(service.IsType(PANEL_TYPE_SERVICE));
  EXPECT_EQ(0, panel_service_get_n_indicators(service));
}

TEST_F(TestPanelService, Destruction)
{
  gpointer weak_ptr = reinterpret_cast<gpointer>(0xdeadbeef);
  g_object_add_weak_pointer(glib::object_cast<GObject>(service), &weak_ptr);
  ASSERT_THAT(weak_ptr, NotNull());
  service = nullptr;
  EXPECT_THAT(weak_ptr, IsNull());
}

TEST_F(TestPanelService, Singleton)
{
  ASSERT_EQ(service, panel_service_get_default());
  EXPECT_EQ(1, G_OBJECT(service.RawPtr())->ref_count);
}

TEST_F(TestPanelService, IndicatorLoading)
{
  glib::Object<IndicatorObject> object(mock_indicator_object_new());
  ASSERT_TRUE(object.IsType(INDICATOR_OBJECT_TYPE));
  ASSERT_TRUE(object.IsType(MOCK_TYPE_INDICATOR_OBJECT));

  panel_service_add_indicator(service, object);
  ASSERT_EQ(1, panel_service_get_n_indicators(service));

  EXPECT_EQ(object, panel_service_get_indicator_nth(service, 0));
}

TEST_F(TestPanelService, ManyIndicatorsLoading)
{
  glib::Object<IndicatorObject> object;

  for (unsigned i = 0; i < 20; ++i)
  {
    object = mock_indicator_object_new();
    panel_service_add_indicator(service, object);

    ASSERT_EQ(i+1, panel_service_get_n_indicators(service));
    ASSERT_EQ(object, panel_service_get_indicator_nth(service, i));
  }
}

TEST_F(TestPanelService, EmptyIndicatorObject)
{
  glib::Object<IndicatorObject> object(mock_indicator_object_new());
  panel_service_add_indicator(service, object);

  glib::Variant result(panel_service_sync(service));
  ASSERT_TRUE(result);

  EXPECT_EQ(1, GetIndicatorsInResult(result));
  EXPECT_EQ(0, GetEntriesInResult(result));
}

TEST_F(TestPanelService, ManyEmptyIndicatorObjects)
{
  glib::Object<IndicatorObject> object;

  for (unsigned i = 0; i < 20; ++i)
  {
    object = mock_indicator_object_new();
    panel_service_add_indicator(service, object);
    glib::Variant result(panel_service_sync(service));
    ASSERT_TRUE(result);

    ASSERT_EQ(i+1, GetIndicatorsInResult(result));
    ASSERT_EQ(0, GetEntriesInResult(result));
  }
}

TEST_F(TestPanelService, EntryAddition)
{
  glib::Object<IndicatorObject> object(mock_indicator_object_new());
  auto mock_object = glib::object_cast<MockIndicatorObject>(object);

  mock_indicator_object_add_entry(mock_object, "Hello", "gtk-apply");
  panel_service_add_indicator(service, object);

  glib::Variant result(panel_service_sync(service));
  EXPECT_EQ(1, GetIndicatorsInResult(result));
  EXPECT_EQ(1, GetEntriesInResult(result));
}

TEST_F(TestPanelService, ManyEntriesAddition)
{
  glib::Object<IndicatorObject> object(mock_indicator_object_new());
  auto mock_object = glib::object_cast<MockIndicatorObject>(object);
  panel_service_add_indicator(service, object);

  for (unsigned i = 0; i < 20; ++i)
  {
    mock_indicator_object_add_entry(mock_object, ("Entry"+std::to_string(i)).c_str(), "gtk-forward");
    glib::Variant result(panel_service_sync(service));
    ASSERT_EQ(1, GetIndicatorsInResult(result));
    ASSERT_EQ(i+1, GetEntriesInResult(result));
  }
}

} // anonymous namespace
