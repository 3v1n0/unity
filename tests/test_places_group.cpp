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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 *
 */

#include "config.h"
#include <gmock/gmock.h>
using namespace testing;

#include <Nux/Nux.h>
#include <Nux/PaintLayer.h>

#include "PlacesGroup.h"
using namespace unity;
using namespace unity::dash;

namespace {

class MockDashStyle : public dash::StyleInterface
{
public:
  MockDashStyle()
  {
    base_texture_.Adopt(nux::CreateTexture2DFromFile(SOURCEDATADIR "/album_missing.png", true, -1));
  }

  MOCK_METHOD2(FocusOverlay, nux::AbstractPaintLayer*(int width, int height));
  MOCK_CONST_METHOD0(GetCategoryBackground, nux::ObjectPtr<nux::BaseTexture> const&());
  MOCK_CONST_METHOD0(GetCategoryBackgroundNoFilters, nux::ObjectPtr<nux::BaseTexture> const&());

  MOCK_CONST_METHOD0(GetGroupExpandIcon, nux::ObjectPtr<nux::BaseTexture> const&());
  MOCK_CONST_METHOD0(GetGroupUnexpandIcon, nux::ObjectPtr<nux::BaseTexture> const&());

  MOCK_CONST_METHOD0(GetCategoryIconSize, RawPixel());
  MOCK_CONST_METHOD0(GetCategoryHeaderLeftPadding, RawPixel());

  MOCK_CONST_METHOD0(GetPlacesGroupTopSpace, RawPixel());
  MOCK_CONST_METHOD0(GetPlacesGroupResultTopPadding, RawPixel());
  MOCK_CONST_METHOD0(GetPlacesGroupResultLeftPadding, RawPixel());

  nux::ObjectPtr<nux::BaseTexture> base_texture_;
};


class TestPlacesGroup : public Test
{
public:
  void SetUp()
  {
    SetupMockDashStyle();

    places_group_ = new PlacesGroup(dash_style_);
  }

  void SetupMockDashStyle()
  {
    ON_CALL(dash_style_, FocusOverlay(_, _))
      .WillByDefault(Return(new nux::ColorLayer(nux::color::White)));

    ON_CALL(Const(dash_style_), GetCategoryBackground())
      .WillByDefault(ReturnRef(dash_style_.base_texture_));

    ON_CALL(Const(dash_style_), GetCategoryBackgroundNoFilters())
      .WillByDefault(ReturnRef(dash_style_.base_texture_));

    ON_CALL(Const(dash_style_), GetGroupExpandIcon())
      .WillByDefault(ReturnRef(dash_style_.base_texture_));

    ON_CALL(Const(dash_style_), GetGroupUnexpandIcon())
         .WillByDefault(ReturnRef(dash_style_.base_texture_));

    ON_CALL(dash_style_, GetCategoryHeaderLeftPadding())
         .WillByDefault(Return(19_em));

    ON_CALL(dash_style_, GetPlacesGroupTopSpace())
         .WillByDefault(Return(7_em));

    ON_CALL(dash_style_, GetCategoryIconSize())
         .WillByDefault(Return(0_em));

    ON_CALL(dash_style_, GetPlacesGroupResultTopPadding())
         .WillByDefault(Return(0_em));

    ON_CALL(dash_style_, GetPlacesGroupResultLeftPadding())
         .WillByDefault(Return(0_em));
  }

  NiceMock<MockDashStyle> dash_style_;
  nux::ObjectPtr<PlacesGroup> places_group_;
};


TEST_F(TestPlacesGroup, Constructor)
{
  EXPECT_CALL(dash_style_, GetGroupExpandIcon())
    .Times(1);

  EXPECT_CALL(dash_style_, GetGroupUnexpandIcon())
    .Times(0);

  PlacesGroup places_group(dash_style_);

  EXPECT_FALSE(places_group.GetExpanded());
}

}
