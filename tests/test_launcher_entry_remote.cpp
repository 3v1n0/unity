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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <gmock/gmock.h>

#include "LauncherEntryRemote.h"

using namespace std;
using namespace unity;
using namespace testing;

namespace
{

GVariant* BuildVariantParameters(std::string const& app_uri = "app_uri",
                                 std::string const& emblem = "emblem",
                                 bool emblem_visible = false,
                                 long long count = 0,
                                 bool count_visible = false,
                                 double progress = 0.0f,
                                 bool progress_visible = false,
                                 bool urgent = false,
                                 std::string const& quicklist_path = "/my/quicklist/path")
{
  GVariantBuilder b;

  g_variant_builder_init(&b, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_add(&b, "{sv}", "emblem", g_variant_new_string(emblem.c_str()));
  g_variant_builder_add(&b, "{sv}", "emblem-visible", g_variant_new_boolean(emblem_visible));
  g_variant_builder_add(&b, "{sv}", "count", g_variant_new_int64(count));
  g_variant_builder_add(&b, "{sv}", "count-visible", g_variant_new_boolean(count_visible));
  g_variant_builder_add(&b, "{sv}", "progress", g_variant_new_double(progress));
  g_variant_builder_add(&b, "{sv}", "progress-visible", g_variant_new_boolean(progress_visible));
  g_variant_builder_add(&b, "{sv}", "urgent", g_variant_new_boolean(urgent));
  g_variant_builder_add(&b, "{sv}", "quicklist", g_variant_new_string(quicklist_path.c_str()));

  return g_variant_new("(sa{sv})", app_uri.c_str(), &b);
}

TEST(TestLauncherEntryRemote, DummyConstruction)
{
  LauncherEntryRemote entry("TestName", nullptr);

  EXPECT_EQ(entry.DBusName(), "TestName");
  EXPECT_TRUE(entry.AppUri().empty());
  EXPECT_TRUE(entry.Emblem().empty());
  EXPECT_EQ(entry.Count(), 0);
  EXPECT_EQ(entry.Progress(), 0.0f);
  EXPECT_THAT(entry.Quicklist().RawPtr(), IsNull());
  EXPECT_FALSE(entry.EmblemVisible());
  EXPECT_FALSE(entry.CountVisible());
  EXPECT_FALSE(entry.ProgressVisible());
  EXPECT_FALSE(entry.Urgent());
}

TEST(TestLauncherEntryRemote, Construction)
{
  LauncherEntryRemote entry("TestName", BuildVariantParameters());

  EXPECT_EQ(entry.DBusName(), "TestName");
  EXPECT_EQ(entry.AppUri(), "app_uri");
  EXPECT_EQ(entry.Emblem(), "emblem");
  EXPECT_EQ(entry.Count(), 0);
  EXPECT_EQ(entry.Progress(), 0.0f);
  EXPECT_THAT(entry.Quicklist().RawPtr(), NotNull());
  EXPECT_FALSE(entry.EmblemVisible());
  EXPECT_FALSE(entry.CountVisible());
  EXPECT_FALSE(entry.ProgressVisible());
  EXPECT_FALSE(entry.Urgent());
}

TEST(TestLauncherEntryRemote, CustomConstruction)
{
  GVariant* parameters;
  parameters = BuildVariantParameters("Uri", "TestEmblem", true, 55, true, 31.12f,
                                      true, true, "/My/DBus/Menu/For/QL");
  LauncherEntryRemote entry("CustomName", parameters);

  EXPECT_EQ(entry.DBusName(), "CustomName");
  EXPECT_EQ(entry.AppUri(), "Uri");
  EXPECT_EQ(entry.Emblem(), "TestEmblem");
  EXPECT_EQ(entry.Count(), 55);
  EXPECT_EQ(entry.Progress(), 31.12f);
  EXPECT_THAT(entry.Quicklist().RawPtr(), NotNull());
  EXPECT_TRUE(entry.EmblemVisible());
  EXPECT_TRUE(entry.CountVisible());
  EXPECT_TRUE(entry.ProgressVisible());
  EXPECT_TRUE(entry.Urgent());
}

TEST(TestLauncherEntryRemote, UpdateFromOther)
{
  LauncherEntryRemote entry1("Entry1", BuildVariantParameters("AppURI1"));

  ASSERT_EQ(entry1.DBusName(), "Entry1");
  ASSERT_EQ(entry1.AppUri(), "AppURI1");
  auto old_ql1 = entry1.Quicklist();
  ASSERT_THAT(old_ql1.RawPtr(), NotNull());

  GVariant* parameters;
  parameters = BuildVariantParameters("Uri2", "TestEmblem", false, 5, true, 0.12f,
                                      true, false, "/My/DBus/Menu/For/QL");

  LauncherEntryRemote::Ptr entry2(new LauncherEntryRemote("Entry2", parameters));
  ASSERT_EQ(entry2->AppUri(), "Uri2");

  entry1.Update(entry2);

  EXPECT_EQ(entry1.DBusName(), "Entry2");
  EXPECT_EQ(entry1.AppUri(), "AppURI1");
  EXPECT_EQ(entry1.Emblem(), "TestEmblem");
  EXPECT_EQ(entry1.Count(), 5);
  EXPECT_EQ(entry1.Progress(), 0.12f);
  EXPECT_THAT(entry1.Quicklist().RawPtr(), NotNull());
  EXPECT_NE(old_ql1, entry1.Quicklist());
  EXPECT_FALSE(entry1.EmblemVisible());
  EXPECT_TRUE(entry1.CountVisible());
  EXPECT_TRUE(entry1.ProgressVisible());
  EXPECT_FALSE(entry1.Urgent());
}

TEST(TestLauncherEntryRemote, UpdateFromVariantIter)
{
  LauncherEntryRemote entry1("Entry1", BuildVariantParameters("AppURI1"));

  ASSERT_EQ(entry1.DBusName(), "Entry1");
  ASSERT_EQ(entry1.AppUri(), "AppURI1");
  auto old_ql1 = entry1.Quicklist();
  ASSERT_THAT(old_ql1.RawPtr(), NotNull());

  GVariant* parameters;
  parameters = BuildVariantParameters("Uri2", "TestEmblem", false, 5, true, 0.12f,
                                      true, false, "/My/DBus/Menu/For/QL");

  gchar* app_uri;
  GVariantIter* prop_iter;
  g_variant_get(parameters, "(&sa{sv})", &app_uri, &prop_iter);
  entry1.Update(prop_iter);
  g_variant_iter_free(prop_iter);
  g_variant_unref(parameters);

  EXPECT_EQ(entry1.DBusName(), "Entry1");
  EXPECT_EQ(entry1.AppUri(), "AppURI1");
  EXPECT_EQ(entry1.Emblem(), "TestEmblem");
  EXPECT_EQ(entry1.Count(), 5);
  EXPECT_EQ(entry1.Progress(), 0.12f);
  EXPECT_THAT(entry1.Quicklist().RawPtr(), NotNull());
  EXPECT_NE(old_ql1, entry1.Quicklist());
  EXPECT_FALSE(entry1.EmblemVisible());
  EXPECT_TRUE(entry1.CountVisible());
  EXPECT_TRUE(entry1.ProgressVisible());
  EXPECT_FALSE(entry1.Urgent());
}

TEST(TestLauncherEntryRemote, ChangeDBusName)
{
  LauncherEntryRemote entry("Entry", BuildVariantParameters("AppURI"));

  bool name_changed = false;
  std::string old_name;
  entry.dbus_name_changed.connect([&] (LauncherEntryRemote*, std::string s) {
    name_changed = true;
    old_name = s;
  });

  auto old_ql = entry.Quicklist();
  ASSERT_THAT(old_ql.RawPtr(), NotNull());
  ASSERT_EQ(entry.DBusName(), "Entry");

  entry.SetDBusName("NewEntryName");
  ASSERT_EQ(entry.DBusName(), "NewEntryName");

  EXPECT_THAT(entry.Quicklist().RawPtr(), IsNull());
  EXPECT_NE(old_ql, entry.Quicklist());

  EXPECT_TRUE(name_changed);
  EXPECT_EQ(old_name, "Entry");
}

TEST(TestLauncherEntryRemote, EmblemChangedSignal)
{
  GVariant* parameters;
  parameters = BuildVariantParameters("Uri", "TestEmblem1", true, 55, true, 31.12f,
                                      true, true, "/My/DBus/Menu/For/QL");

  LauncherEntryRemote entry1("Entry1", parameters);

  bool value_changed = false;
  entry1.emblem_changed.connect([&] (LauncherEntryRemote*) { value_changed = true; });

  parameters = BuildVariantParameters("Uri", "TestEmblem2", true, 55, true, 31.12f,
                                      true, true, "/My/DBus/Menu/For/QL");

  LauncherEntryRemote::Ptr entry2(new LauncherEntryRemote("Entry2", parameters));

  ASSERT_EQ(entry1.Emblem(), "TestEmblem1");

  entry1.Update(entry2);

  EXPECT_TRUE(value_changed);
  EXPECT_EQ(entry1.Emblem(), "TestEmblem2");
}

TEST(TestLauncherEntryRemote, CountChangedSignal)
{
  GVariant* parameters;
  parameters = BuildVariantParameters("Uri", "TestEmblem", true, 55, true, 31.12f,
                                      true, true, "/My/DBus/Menu/For/QL");

  LauncherEntryRemote entry1("Entry1", parameters);

  bool value_changed = false;
  entry1.count_changed.connect([&] (LauncherEntryRemote*) { value_changed = true; });

  parameters = BuildVariantParameters("Uri", "TestEmblem", true, 155, true, 31.12f,
                                      true, true, "/My/DBus/Menu/For/QL");

  LauncherEntryRemote::Ptr entry2(new LauncherEntryRemote("Entry2", parameters));

  ASSERT_EQ(entry1.Count(), 55);

  entry1.Update(entry2);

  EXPECT_TRUE(value_changed);
  EXPECT_EQ(entry1.Count(), 155);
}

TEST(TestLauncherEntryRemote, ProgressChangedSignal)
{
  GVariant* parameters;
  parameters = BuildVariantParameters("Uri", "TestEmblem", true, 55, true, 0.12f,
                                      true, true, "/My/DBus/Menu/For/QL");

  LauncherEntryRemote entry1("Entry1", parameters);

  bool value_changed = false;
  entry1.progress_changed.connect([&] (LauncherEntryRemote*) { value_changed = true; });

  parameters = BuildVariantParameters("Uri", "TestEmblem", true, 55, true, 31.12f,
                                      true, true, "/My/DBus/Menu/For/QL");

  LauncherEntryRemote::Ptr entry2(new LauncherEntryRemote("Entry2", parameters));

  ASSERT_EQ(entry1.Progress(), 0.12f);

  entry1.Update(entry2);

  EXPECT_TRUE(value_changed);
  EXPECT_EQ(entry1.Progress(), 31.12f);
}

TEST(TestLauncherEntryRemote, QuicklistChangedSignal)
{
  GVariant* parameters;
  parameters = BuildVariantParameters("Uri", "TestEmblem", true, 55, true, 31.12f,
                                      true, true, "/My/DBus/Menu/For/QL1");

  LauncherEntryRemote entry1("Entry1", parameters);

  bool value_changed = false;
  entry1.quicklist_changed.connect([&] (LauncherEntryRemote*) { value_changed = true; });

  parameters = BuildVariantParameters("Uri", "TestEmblem", true, 55, true, 31.12f,
                                      true, true, "/My/DBus/Menu/For/QL2");

  LauncherEntryRemote::Ptr entry2(new LauncherEntryRemote("Entry2", parameters));

  auto old_ql1 = entry1.Quicklist();
  ASSERT_THAT(old_ql1.RawPtr(), NotNull());

  entry1.Update(entry2);

  EXPECT_TRUE(value_changed);
  EXPECT_NE(entry1.Quicklist(), old_ql1);
  EXPECT_EQ(entry1.Quicklist(), entry2->Quicklist());
}

TEST(TestLauncherEntryRemote, EmblemVisibilityChanged)
{
  GVariant* parameters;
  parameters = BuildVariantParameters("Uri", "TestEmblem", false, 55, true, 31.12f,
                                      true, true, "/My/DBus/Menu/For/QL");

  LauncherEntryRemote entry1("Entry1", parameters);

  bool value_changed = false;
  entry1.emblem_visible_changed.connect([&] (LauncherEntryRemote*) { value_changed = true; });

  parameters = BuildVariantParameters("Uri", "TestEmblem", true, 55, true, 31.12f,
                                      true, true, "/My/DBus/Menu/For/QL");

  LauncherEntryRemote::Ptr entry2(new LauncherEntryRemote("Entry2", parameters));

  ASSERT_FALSE(entry1.EmblemVisible());

  entry1.Update(entry2);

  EXPECT_TRUE(value_changed);
  EXPECT_TRUE(entry1.EmblemVisible());
}

TEST(TestLauncherEntryRemote, CountVisibilityChanged)
{
  GVariant* parameters;
  parameters = BuildVariantParameters("Uri", "TestEmblem", false, 55, false, 31.12f,
                                      true, true, "/My/DBus/Menu/For/QL");

  LauncherEntryRemote entry1("Entry1", parameters);

  bool value_changed = false;
  entry1.count_visible_changed.connect([&] (LauncherEntryRemote*) { value_changed = true; });

  parameters = BuildVariantParameters("Uri", "TestEmblem", false, 55, true, 31.12f,
                                      true, true, "/My/DBus/Menu/For/QL");

  LauncherEntryRemote::Ptr entry2(new LauncherEntryRemote("Entry2", parameters));

  ASSERT_FALSE(entry1.CountVisible());

  entry1.Update(entry2);

  EXPECT_TRUE(value_changed);
  EXPECT_TRUE(entry1.CountVisible());
}

TEST(TestLauncherEntryRemote, ProgressVisibilityChanged)
{
  GVariant* parameters;
  parameters = BuildVariantParameters("Uri", "TestEmblem", false, 55, false, 31.12f,
                                      false, true, "/My/DBus/Menu/For/QL");

  LauncherEntryRemote entry1("Entry1", parameters);

  bool value_changed = false;
  entry1.progress_visible_changed.connect([&] (LauncherEntryRemote*) { value_changed = true; });

  parameters = BuildVariantParameters("Uri", "TestEmblem", false, 55, false, 31.12f,
                                      true, true, "/My/DBus/Menu/For/QL");

  LauncherEntryRemote::Ptr entry2(new LauncherEntryRemote("Entry2", parameters));

  ASSERT_FALSE(entry1.ProgressVisible());

  entry1.Update(entry2);

  EXPECT_TRUE(value_changed);
  EXPECT_TRUE(entry1.ProgressVisible());
}

TEST(TestLauncherEntryRemote, UrgentChanged)
{
  GVariant* parameters;
  parameters = BuildVariantParameters("Uri", "TestEmblem", false, 55, false, 31.12f,
                                      false, false, "/My/DBus/Menu/For/QL");

  LauncherEntryRemote entry1("Entry1", parameters);

  bool value_changed = false;
  entry1.urgent_changed.connect([&] (LauncherEntryRemote*) { value_changed = true; });

  parameters = BuildVariantParameters("Uri", "TestEmblem", false, 55, false, 31.12f,
                                      false, true, "/My/DBus/Menu/For/QL");

  LauncherEntryRemote::Ptr entry2(new LauncherEntryRemote("Entry2", parameters));

  ASSERT_FALSE(entry1.Urgent());

  entry1.Update(entry2);

  EXPECT_TRUE(value_changed);
  EXPECT_TRUE(entry1.Urgent());
}

} // Namespace
