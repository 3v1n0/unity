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
  MOCK_METHOD0(GetCategoryBackground, nux::BaseTexture*());
  MOCK_METHOD0(GetCategoryBackgroundNoFilters, nux::BaseTexture*());

  MOCK_METHOD0(GetGroupExpandIcon, nux::BaseTexture*());
  MOCK_METHOD0(GetGroupUnexpandIcon, nux::BaseTexture*());

  MOCK_CONST_METHOD0(GetCategoryIconSize, int());
  MOCK_CONST_METHOD0(GetCategoryHeaderLeftPadding, int());

  MOCK_CONST_METHOD0(GetPlacesGroupTopSpace, int());
  MOCK_CONST_METHOD0(GetPlacesGroupResultTopPadding, int());
  MOCK_CONST_METHOD0(GetPlacesGroupResultLeftPadding, int());

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

    ON_CALL(dash_style_, GetCategoryBackground())
      .WillByDefault(Return(dash_style_.base_texture_.GetPointer()));

    ON_CALL(dash_style_, GetCategoryBackgroundNoFilters())
      .WillByDefault(Return(dash_style_.base_texture_.GetPointer()));

    ON_CALL(dash_style_, GetGroupExpandIcon())
      .WillByDefault(Return(dash_style_.base_texture_.GetPointer()));

    ON_CALL(dash_style_, GetGroupUnexpandIcon())
         .WillByDefault(Return(dash_style_.base_texture_.GetPointer()));

    ON_CALL(dash_style_, GetCategoryHeaderLeftPadding())
         .WillByDefault(Return(19));

    ON_CALL(dash_style_, GetPlacesGroupTopSpace())
         .WillByDefault(Return(7));
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
