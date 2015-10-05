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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <gtk/gtk.h>
#include <gmock/gmock.h>
#include <UnityCore/GLibWrapper.h>

using namespace unity;
using namespace testing;

namespace
{

TEST(TestGtkIconInfo, EmptyIconInfo)
{
  glib::Object<GtkIconInfo> info;
  EXPECT_THAT(info.RawPtr(), IsNull());
  EXPECT_FALSE(info);
  EXPECT_EQ(info, nullptr);
}

TEST(TestGtkIconInfo, ValidIconInfo)
{
  GList *icons = gtk_icon_theme_list_icons(gtk_icon_theme_get_default(), "Emblems");

  for (GList *l = icons; l; l = l->next)
  {
    auto icon_name = static_cast <const char*>(l->data);
    GtkIconInfo *ginfo = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(), icon_name, 32, GTK_ICON_LOOKUP_FORCE_SIZE);
    glib::Object<GtkIconInfo> info(ginfo);
    ASSERT_THAT(info.RawPtr(), NotNull());
    ASSERT_TRUE(info);
    ASSERT_EQ(info, ginfo);
  }

  g_list_free_full(icons, g_free);
}

}
