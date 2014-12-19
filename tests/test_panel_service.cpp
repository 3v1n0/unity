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
#include "panel-service-private.h"
#include "mock_indicator_object.h"

using namespace testing;
using namespace unity;

namespace
{
typedef std::tuple<glib::Object<GtkLabel>, glib::Object<GtkImage>> EntryObjects;

const std::string SYNC_ENTRY_VARIANT_FORMAT = ENTRY_SIGNATURE;
const std::string SYNC_ENTRIES_VARIANT_FORMAT = "(" ENTRY_ARRAY_SIGNATURE ")";

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
    uint32_t parent_window;
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
    guint32 parent_window;
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
                               &parent_window,
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
                          parent_window,
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

  bool IsGObjectConectedTo(gpointer object, gpointer data)
  {
    return g_signal_handler_find(object, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, data) != 0;
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

TEST_F(TestPanelService, EntryIndicatorObjectEntryAddition)
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

TEST_F(TestPanelService, ManyEntriesIndicatorObjectEntryAddition)
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

TEST_F(TestPanelService, EntryRemovalIndicatorObject)
{
  glib::Object<IndicatorObject> object(mock_indicator_object_new());
  auto mock_object = glib::object_cast<MockIndicatorObject>(object);

  auto* entry = mock_indicator_object_add_entry(mock_object, "Hello", "gtk-apply");
  panel_service_add_indicator(service, object);

  glib::Variant result(panel_service_sync(service));
  ASSERT_EQ(1, GetIndicatorsInResult(result));
  ASSERT_EQ(1, GetEntriesInResult(result));

  glib::Object<GtkLabel> label(entry->label, glib::AddRef());
  glib::Object<GtkImage> icon(entry->image, glib::AddRef());
  mock_indicator_object_remove_entry(mock_object, entry);

  result = panel_service_sync(service);
  EXPECT_EQ(1, GetIndicatorsInResult(result));
  EXPECT_EQ(0, GetEntriesInResult(result));

  EXPECT_FALSE(IsGObjectConectedTo(label, object));
  EXPECT_FALSE(IsGObjectConectedTo(icon, object));
}

TEST_F(TestPanelService, ManyEntriesRemovalIndicatorObject)
{
  std::vector<EntryObjects> entries_objs;
  glib::Object<IndicatorObject> object(mock_indicator_object_new());
  auto mock_object = glib::object_cast<MockIndicatorObject>(object);
  panel_service_add_indicator(service, object);

  for (unsigned i = 0; i < 20; ++i)
  {
    auto* entry = mock_indicator_object_add_entry(mock_object, ("Entry"+std::to_string(i)).c_str(), "gtk-forward");
    glib::Object<GtkLabel> label(entry->label, glib::AddRef());
    glib::Object<GtkImage> icon(entry->image, glib::AddRef());
    entries_objs.push_back(std::make_tuple(label, icon));

    mock_indicator_object_remove_entry(mock_object, entry);
  }

  glib::Variant result(panel_service_sync(service));

  ASSERT_EQ(1, GetIndicatorsInResult(result));
  ASSERT_EQ(0, GetEntriesInResult(result));

  for (auto const& entry_objs : entries_objs)
  {
    ASSERT_FALSE(IsGObjectConectedTo(std::get<0>(entry_objs), object));
    ASSERT_FALSE(IsGObjectConectedTo(std::get<1>(entry_objs), object));
  }
}

TEST_F(TestPanelService, EntryIndicatorObjectRemoval)
{
  glib::Object<IndicatorObject> object(mock_indicator_object_new());
  auto mock_object = glib::object_cast<MockIndicatorObject>(object);

  auto* entry = mock_indicator_object_add_entry(mock_object, "Hello", "gtk-apply");
  panel_service_add_indicator(service, object);

  glib::Variant result(panel_service_sync(service));
  ASSERT_EQ(1, GetIndicatorsInResult(result));
  ASSERT_EQ(1, GetEntriesInResult(result));

  glib::Object<GtkLabel> label(entry->label, glib::AddRef());
  glib::Object<GtkImage> icon(entry->image, glib::AddRef());

  panel_service_remove_indicator(service, object);
  result = panel_service_sync(service);
  EXPECT_EQ(1, GetIndicatorsInResult(result));
  EXPECT_EQ(0, GetEntriesInResult(result));

  EXPECT_FALSE(IsGObjectConectedTo(label, object));
  EXPECT_FALSE(IsGObjectConectedTo(icon, object));
}

TEST_F(TestPanelService, ManyEntriesIndicatorObjectRemoval)
{
  std::vector<EntryObjects> entries_objs;
  glib::Object<IndicatorObject> object(mock_indicator_object_new());
  auto mock_object = glib::object_cast<MockIndicatorObject>(object);
  panel_service_add_indicator(service, object);

  for (unsigned i = 0; i < 20; ++i)
  {
    auto* entry = mock_indicator_object_add_entry(mock_object, ("Entry"+std::to_string(i)).c_str(), "");
    glib::Object<GtkLabel> label(entry->label, glib::AddRef());
    glib::Object<GtkImage> icon(entry->image, glib::AddRef());
    entries_objs.push_back(std::make_tuple(label, icon));
  }

  glib::Variant result(panel_service_sync(service));
  ASSERT_EQ(1, GetIndicatorsInResult(result));
  ASSERT_EQ(20, GetEntriesInResult(result));

  panel_service_remove_indicator(service, object);
  result = panel_service_sync(service);
  EXPECT_EQ(1, GetIndicatorsInResult(result));
  EXPECT_EQ(0, GetEntriesInResult(result));

  for (auto const& entry_objs : entries_objs)
  {
    ASSERT_FALSE(IsGObjectConectedTo(std::get<0>(entry_objs), object));
    ASSERT_FALSE(IsGObjectConectedTo(std::get<1>(entry_objs), object));
  }
}

TEST_F(TestPanelService, ManyEntriesIndicatorsObjectAddition)
{
  for (unsigned i = 0; i < 20; ++i)
  {
    glib::Object<IndicatorObject> object(mock_indicator_object_new());
    auto mock_object = glib::object_cast<MockIndicatorObject>(object);
    panel_service_add_indicator(service, object);

    for (unsigned j = 0; j < 20; ++j)
      mock_indicator_object_add_entry(mock_object, ("Entry"+std::to_string(j)).c_str(), "");
  }

  glib::Variant result(panel_service_sync(service));
  EXPECT_EQ(20, GetIndicatorsInResult(result));
  EXPECT_EQ(400, GetEntriesInResult(result));
}

TEST_F(TestPanelService, ManyEntriesIndicatorsObjectClear)
{
  for (unsigned i = 0; i < 20; ++i)
  {
    glib::Object<IndicatorObject> object(mock_indicator_object_new());
    auto mock_object = glib::object_cast<MockIndicatorObject>(object);
    panel_service_add_indicator(service, object);

    for (unsigned j = 0; j < 20; ++j)
      mock_indicator_object_add_entry(mock_object, ("Entry"+std::to_string(i)).c_str(), "");
  }

  glib::Variant result(panel_service_sync(service));
  ASSERT_EQ(20, GetIndicatorsInResult(result));
  ASSERT_EQ(400, GetEntriesInResult(result));

  panel_service_clear_indicators(service);
  result = panel_service_sync(service);
  EXPECT_EQ(20, GetIndicatorsInResult(result));
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

TEST(TestPanelServiceCompizShortcutParsing, Null)
{
  KeyBinding kb;
  parse_string_keybinding(NULL, &kb);

  EXPECT_EQ(NoSymbol, kb.key);
  EXPECT_EQ(NoSymbol, kb.fallback);
  EXPECT_EQ(0, kb.modifiers);
}

TEST(TestPanelServiceCompizShortcutParsing, Empty)
{
  KeyBinding kb;
  parse_string_keybinding("", &kb);

  EXPECT_EQ(NoSymbol, kb.key);
  EXPECT_EQ(NoSymbol, kb.fallback);
  EXPECT_EQ(0, kb.modifiers);
}

TEST(TestPanelServiceCompizShortcutParsing, SimpleKey)
{
  KeyBinding kb;
  parse_string_keybinding("U", &kb);

  EXPECT_EQ(XK_U, kb.key);
  EXPECT_EQ(NoSymbol, kb.fallback);
  EXPECT_EQ(0, kb.modifiers);
}

TEST(TestPanelServiceCompizShortcutParsing, ControlCombo)
{
  KeyBinding kb;
  parse_string_keybinding("<Control>F1", &kb);

  EXPECT_EQ(XK_F1, kb.key);
  EXPECT_EQ(NoSymbol, kb.fallback);
  EXPECT_EQ(ControlMask, kb.modifiers);
}

TEST(TestPanelServiceCompizShortcutParsing, AltCombo)
{
  KeyBinding kb;
  parse_string_keybinding("<Alt>F2", &kb);

  EXPECT_EQ(XK_F2, kb.key);
  EXPECT_EQ(NoSymbol, kb.fallback);
  EXPECT_EQ(AltMask, kb.modifiers);
}

TEST(TestPanelServiceCompizShortcutParsing, ShiftCombo)
{
  KeyBinding kb;
  parse_string_keybinding("<Shift>F3", &kb);

  EXPECT_EQ(XK_F3, kb.key);
  EXPECT_EQ(NoSymbol, kb.fallback);
  EXPECT_EQ(ShiftMask, kb.modifiers);
}

TEST(TestPanelServiceCompizShortcutParsing, SuperCombo)
{
  KeyBinding kb;
  parse_string_keybinding("<Super>F4", &kb);

  EXPECT_EQ(XK_F4, kb.key);
  EXPECT_EQ(NoSymbol, kb.fallback);
  EXPECT_EQ(SuperMask, kb.modifiers);
}

TEST(TestPanelServiceCompizShortcutParsing, FullCombo)
{
  KeyBinding kb;
  parse_string_keybinding("<Control><Alt><Shift><Super>Escape", &kb);

  EXPECT_EQ(XK_Escape, kb.key);
  EXPECT_EQ(NoSymbol, kb.fallback);
  EXPECT_EQ(ControlMask|AltMask|ShiftMask|SuperMask, kb.modifiers);
}

TEST(TestPanelServiceCompizShortcutParsing, MetaKeyControl)
{
  KeyBinding kb;
  parse_string_keybinding("<Control>", &kb);

  EXPECT_EQ(XK_Control_L, kb.key);
  EXPECT_EQ(XK_Control_R, kb.fallback);
  EXPECT_EQ(0, kb.modifiers);
}

TEST(TestPanelServiceCompizShortcutParsing, MetaKeyAlt)
{
  KeyBinding kb;
  parse_string_keybinding("<Alt>", &kb);

  EXPECT_EQ(XK_Alt_L, kb.key);
  EXPECT_EQ(XK_Alt_R, kb.fallback);
  EXPECT_EQ(0, kb.modifiers);
}

TEST(TestPanelServiceCompizShortcutParsing, MetaKeyShift)
{
  KeyBinding kb;
  parse_string_keybinding("<Shift>", &kb);

  EXPECT_EQ(XK_Shift_L, kb.key);
  EXPECT_EQ(XK_Shift_R, kb.fallback);
  EXPECT_EQ(0, kb.modifiers);
}

TEST(TestPanelServiceCompizShortcutParsing, MetaKeySuper)
{
  KeyBinding kb;
  parse_string_keybinding("<Super>", &kb);

  EXPECT_EQ(XK_Super_L, kb.key);
  EXPECT_EQ(XK_Super_R, kb.fallback);
  EXPECT_EQ(0, kb.modifiers);
}

TEST(TestPanelServiceCompizShortcutParsing, MetaKeyMix)
{
  KeyBinding kb;
  parse_string_keybinding("<Control><Alt><Super>", &kb);

  EXPECT_EQ(XK_Super_L, kb.key);
  EXPECT_EQ(XK_Super_R, kb.fallback);
  EXPECT_EQ(ControlMask|AltMask, kb.modifiers);
}

} // anonymous namespace
