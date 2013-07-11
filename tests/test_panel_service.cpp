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
#include <UnityCore/GLibSignal.h>
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

  struct SyncResult
  {
    std::string indicator_id;
    std::string entry_id;
    std::string entry_name_hint;
    std::string label;
    bool label_sensitive;
    bool label_visible;
    uint32_t image_type;
    std::string image_data;
    bool image_sensitive;
    bool image_visible;
    int32_t priority;
  };

  std::vector<SyncResult> GetResults(glib::Variant const& result) const
  {
    std::vector<SyncResult> results;
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
    gint32 priority;

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
      results.push_back({ glib::gchar_to_string(indicator_id),
                          glib::gchar_to_string(entry_id),
                          glib::gchar_to_string(entry_name_hint),
                          glib::gchar_to_string(label),
                          label_sensitive != FALSE,
                          label_visible != FALSE,
                          image_type,
                          glib::gchar_to_string(image_data),
                          image_sensitive != FALSE,
                          image_visible != FALSE,
                          priority });
    }
    g_variant_iter_free(iter);

    return results;
  }

  std::size_t GetIndicatorsInResult(glib::Variant const& result) const
  {
    EXPECT_TRUE(result);
    if (!result)
      return 0;

    std::set<std::string> objects;

    for (auto const& res : GetResults(result))
    {
      if (!res.indicator_id.empty())
        objects.insert(res.indicator_id);
    }

    return objects.size();
  }

  std::size_t GetEntriesInResult(glib::Variant const& result) const
  {
    EXPECT_TRUE(result);
    if (!result)
      return 0;

    std::set<std::string> entries;

    for (auto const& res : GetResults(result))
    {
      if (!res.entry_id.empty())
        entries.insert(res.entry_id);
    }

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

TEST_F(TestPanelService, EmptyIndicatorObjectAddition)
{
  glib::Object<IndicatorObject> object(mock_indicator_object_new());
  panel_service_add_indicator(service, object);

  glib::Variant result(panel_service_sync(service));
  ASSERT_TRUE(result);

  EXPECT_EQ(1, GetIndicatorsInResult(result));
  EXPECT_EQ(0, GetEntriesInResult(result));
}

TEST_F(TestPanelService, ManyEmptyIndicatorObjectsAddition)
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

TEST_F(TestPanelService, EntryIndicatorObjectAddition)
{
  glib::Object<IndicatorObject> object(mock_indicator_object_new());
  auto mock_object = glib::object_cast<MockIndicatorObject>(object);

  auto* entry = mock_indicator_object_add_entry(mock_object, "Hello", "gtk-apply");
  ASSERT_THAT(entry, NotNull());
  panel_service_add_indicator(service, object);

  glib::Variant result(panel_service_sync(service));
  EXPECT_EQ(1, GetIndicatorsInResult(result));
  EXPECT_EQ(1, GetEntriesInResult(result));
}

TEST_F(TestPanelService, ManyEntriesIndicatorObjectAddition)
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

TEST_F(TestPanelService, EntryIndicatorObjectRemoval)
{
  glib::Object<IndicatorObject> object(mock_indicator_object_new());
  auto mock_object = glib::object_cast<MockIndicatorObject>(object);

  mock_indicator_object_add_entry(mock_object, "Hello", "gtk-apply");
  panel_service_add_indicator(service, object);

  glib::Variant result(panel_service_sync(service));
  ASSERT_EQ(1, GetIndicatorsInResult(result));
  ASSERT_EQ(1, GetEntriesInResult(result));

  panel_service_remove_indicator(service, object);
  result = panel_service_sync(service);
  EXPECT_EQ(1, GetIndicatorsInResult(result));
  EXPECT_EQ(0, GetEntriesInResult(result));
}

TEST_F(TestPanelService, ManyEntriesIndicatorObjectRemoval)
{
  glib::Object<IndicatorObject> object(mock_indicator_object_new());
  auto mock_object = glib::object_cast<MockIndicatorObject>(object);
  panel_service_add_indicator(service, object);

  for (unsigned i = 0; i < 20; ++i)
    mock_indicator_object_add_entry(mock_object, ("Entry"+std::to_string(i)).c_str(), "");

  glib::Variant result(panel_service_sync(service));
  ASSERT_EQ(1, GetIndicatorsInResult(result));
  ASSERT_EQ(20, GetEntriesInResult(result));

  panel_service_remove_indicator(service, object);
  result = panel_service_sync(service);
  EXPECT_EQ(1, GetIndicatorsInResult(result));
  EXPECT_EQ(0, GetEntriesInResult(result));
}

TEST_F(TestPanelService, ActivateRequest)
{
  glib::Object<IndicatorObject> object(mock_indicator_object_new());
  auto mock_object = glib::object_cast<MockIndicatorObject>(object);

  auto* entry = mock_indicator_object_add_entry(mock_object, "Entry", "cmake");
  panel_service_add_indicator(service, object);

  bool called = false;
  std::string const& id = glib::String(g_strdup_printf("%p", entry)).Str();

  glib::Signal<void, PanelService*, const gchar*> activation_signal;
  activation_signal.Connect(service, "entry-activate-request",
  [this, id, &called] (PanelService* srv, const gchar* entry_id) {
    EXPECT_EQ(service, srv);
    EXPECT_EQ(id, entry_id);
    called = true;
  });

  mock_indicator_object_show_entry(mock_object, entry, 1234);
  EXPECT_TRUE(called);
}

} // anonymous namespace
