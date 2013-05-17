// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
#include <gmock/gmock.h>
#include <glib-object.h>

#include "unity-shared/DashStyle.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/GtkUtil.h"
#include "UnityCore/Result.h"
#include "dash/ResultRendererTile.h"

#include "test_utils.h"

using namespace std;
using namespace unity;
using namespace testing;

namespace unity
{

namespace
{

#define DEFAULT_GICON ". GThemedIcon text-x-preview"

GdkPixbuf* GetIconData(std::string icon_hint, int size)
{
  GdkPixbuf *pbuf;
  GtkIconTheme *theme;
  GError *error = NULL;

  theme = gtk_icon_theme_get_default();
  glib::Object<GIcon> icon(g_icon_new_for_string(icon_hint.c_str(), NULL));

  if (icon.IsType(G_TYPE_ICON))
  {
    GtkIconInfo *info = gtk_icon_theme_lookup_by_gicon(theme, icon, size, (GtkIconLookupFlags)0);
    pbuf = gtk_icon_info_load_icon(info, &error);
    if (error != NULL)
    {
      g_error_free (error);
      pbuf = NULL;
    }
    gtk::UnreferenceIconInfo(info);
  }

  return pbuf;
}

} // namespace [anonymous]

class TestResultRenderer : public testing::Test
{
public:
  TestResultRenderer() {}

  unity::Settings settings;
  dash::Style style;
};

class MockResult : public dash::Result
{
public:
  MockResult()
  : Result(NULL, NULL, NULL)
  , renderer_(new dash::TextureContainer())
  {
    ON_CALL (*this, GetURI ()).WillByDefault (Return ("file:///result_render_test"));
    ON_CALL (*this, GetIconHint()).WillByDefault (Return (DEFAULT_GICON));
    ON_CALL (*this, GetCategoryIndex ()).WillByDefault (Return (0));
    ON_CALL (*this, GetName ()).WillByDefault (Return ("Result Render Test"));
    ON_CALL (*this, GetDndURI ()).WillByDefault (Return ("file:///result_render_test_dnd"));
  }

  MOCK_CONST_METHOD0(GetURI, std::string());
  MOCK_CONST_METHOD0(GetIconHint, std::string());
  MOCK_CONST_METHOD0(GetCategoryIndex, std::size_t());
  MOCK_CONST_METHOD0(GetMimeType, std::string());
  MOCK_CONST_METHOD0(GetName, std::string());
  MOCK_CONST_METHOD0(GetComment, std::string());
  MOCK_CONST_METHOD0(GetDndURI, std::string());

  virtual gpointer get_model_tag() const
  {
    return renderer_.get();
  }

private:
  std::auto_ptr<dash::TextureContainer> renderer_;
};

TEST_F(TestResultRenderer, TestConstruction)
{
  dash::ResultRendererTile renderer;
}

TEST_F(TestResultRenderer, TestDndIcon)
{
  dash::ResultRendererTile renderer;
  NiceMock<MockResult> result;

  nux::NBitmapData* bitmap = renderer.GetDndImage(result);
  ASSERT_NE(bitmap, nullptr);
}

}
