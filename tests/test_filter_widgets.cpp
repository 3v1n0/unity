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

#include <list>

#include <gtest/gtest.h>
#include "test_filter_multirange.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/UnitySettings.h"

#include <Nux/Nux.h>
#include <NuxGraphics/Events.h>
#include <NuxCore/ObjectPtr.h>

#include <UnityCore/GLibWrapper.h>
#include "UnityCore/MultiRangeFilter.h"
#include "dash/FilterMultiRangeWidget.h"
#include "dash/FilterMultiRangeButton.h"

using namespace unity;

namespace unity
{
namespace dash
{

class TestFilterMultiRangeWidget : public ::testing::Test
{
public:
  TestFilterMultiRangeWidget()
  : model_(dee_sequence_model_new())
  , filter_widget_(new MockFilterMultiRangeWidget())
  {
    dee_model_set_schema(model_, "s", "s", "s", "s", "a{sv}", "b", "b", "b", NULL);
  }

  void SetFilter(MultiRangeFilter::Ptr const& filter)
  {
    filter_widget_->SetFilter(filter);

    filter_widget_->ComputeContentSize();
    filter_widget_->ComputeContentPosition(0,0);
  }

  class MockFilterMultiRangeWidget : public FilterMultiRangeWidget
  {
  public:
    MockFilterMultiRangeWidget()
    : clicked_(false)
    {
    }

    void ResetState()
    {
      clicked_ = false;
    }

    bool clicked_;

    using FilterMultiRangeWidget::all_button_;
    using FilterMultiRangeWidget::buttons_;
    using FilterMultiRangeWidget::dragging_;

    using InputArea::EmitMouseDownSignal;
    using InputArea::EmitMouseUpSignal;
    using InputArea::EmitMouseDragSignal;

    using FilterMultiRangeWidget::mouse_down_button_;

  protected:

    virtual void Click(FilterMultiRangeButtonPtr const& button)
    {
      clicked_ = true;
      FilterMultiRangeWidget::Click(button);
    }
  };


  static DeeModelIter* AddMultiRangeFilterOptions(glib::Object<DeeModel> const& model)
  {
    std::vector<bool> option_active(6, false);
    GVariantBuilder* builder = nullptr;
    builder = unity::testing::AddFilterHint(builder, "options", unity::testing::AddFilterOptions(option_active));
    GVariant* hints = g_variant_builder_end(builder);

    return dee_model_append(model,
                          "genre",
                          "Genre",
                          "gtk-apply",
                          "genre",
                          hints,
                          TRUE,
                          FALSE, // collapsed
                          FALSE);
  }

  Settings unity_settings_;
  dash::Style dash_style_;

  glib::Object<DeeModel> model_;
  nux::ObjectPtr<MockFilterMultiRangeWidget> filter_widget_;
};

TEST_F(TestFilterMultiRangeWidget, TestConstruction)
{
  MultiRangeFilter::Ptr filter(new MultiRangeFilter(model_, AddMultiRangeFilterOptions(model_)));
  SetFilter(filter);

  ASSERT_EQ(filter_widget_->buttons_.size(), 6);
}

TEST_F(TestFilterMultiRangeWidget, TestClick)
{
  MultiRangeFilter::Ptr filter(new MultiRangeFilter(model_, AddMultiRangeFilterOptions(model_)));
  SetFilter(filter);

  FilterMultiRangeWidget::FilterMultiRangeButtonPtr filter_button = filter_widget_->buttons_[1];
  nux::Geometry geo_button = filter_button->GetAbsoluteGeometry();
  nux::Point center_button1(geo_button.x + geo_button.width/2, geo_button.y + geo_button.height/2);

  filter_widget_->EmitMouseDownSignal(center_button1.x, center_button1.y, NUX_STATE_BUTTON1_DOWN|NUX_EVENT_BUTTON1_DOWN, 0);
  ASSERT_NE(filter_widget_->mouse_down_button_, nullptr);

  filter_widget_->EmitMouseUpSignal(center_button1.y, center_button1.y, NUX_EVENT_BUTTON1_UP, 0);
  ASSERT_EQ(filter_widget_->mouse_down_button_, nullptr);

  EXPECT_EQ(filter_widget_->clicked_, true);
  EXPECT_EQ(filter_button->Active(), true);
  EXPECT_EQ(filter->options()[1]->active, true);
}

TEST_F(TestFilterMultiRangeWidget, TestDrag)
{
  MultiRangeFilter::Ptr filter(new MultiRangeFilter(model_, AddMultiRangeFilterOptions(model_)));
  SetFilter(filter);

  // Genre "1"
  FilterMultiRangeWidget::FilterMultiRangeButtonPtr filter_button1 = filter_widget_->buttons_[1];
  nux::Geometry geo_button1 = filter_button1->GetAbsoluteGeometry();
  nux::Point center_button1(geo_button1.x + geo_button1.width/2, geo_button1.y + geo_button1.height/2);

  // Genre "3"
  FilterMultiRangeWidget::FilterMultiRangeButtonPtr filter_button3 = filter_widget_->buttons_[3];
  nux::Geometry geo_button3 = filter_button3->GetAbsoluteGeometry();
  nux::Point center_button3(geo_button3.x + geo_button3.width/2, geo_button3.y + geo_button3.height/2);

  // Genre "4"
  FilterMultiRangeWidget::FilterMultiRangeButtonPtr filter_button4 = filter_widget_->buttons_[4];
  nux::Geometry geo_button4 = filter_button4->GetAbsoluteGeometry();
  nux::Point center_button4(geo_button4.x + geo_button4.width/2, geo_button4.y + geo_button4.height/2);

  filter_widget_->EmitMouseDownSignal(center_button1.x, center_button1.y, NUX_STATE_BUTTON1_DOWN|NUX_EVENT_BUTTON1_DOWN, 0);

  filter_widget_->EmitMouseDragSignal(center_button4.x, center_button4.y, center_button4.x - center_button1.x, center_button4.y - center_button1.y, NUX_STATE_BUTTON1_DOWN, 0);
  EXPECT_EQ(filter_widget_->dragging_, true);
  unity::testing::ExpectFilterRange(filter, 1, 4);


  // filter_widget_->EmitMouseDragSignal(center_button3.x, center_button3.y, center_button4.x - center_button3.x, center_button4.y - center_button3.y, NUX_STATE_BUTTON1_DOWN, 0);
  // unity::testing::ExpectFilterRange(filter, 1, 3);

  // filter_widget_->EmitMouseUpSignal(center_button3.x, center_button3.y, NUX_EVENT_BUTTON1_UP, 0);
  // EXPECT_EQ(filter_widget_->dragging_, false);
  // unity::testing::ExpectFilterRange(filter, 1, 3);
}

} // namespace dash
} // namespace unity
