/*
 * Copyright 2012-2013 Canonical Ltd.
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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 *
 */

#include "test_switcher_controller.h"

using namespace testing;
using namespace unity;
using namespace unity::switcher;
using namespace std::chrono;

#ifdef ENABLE_DELAYED_TWO_PHASE_CONSTRUCTION_TESTS
TEST_F(TestSwitcherController, LazyConstructionTimeoutLength)
{
  EXPECT_EQ(controller_->GetConstructTimeout(), DEFAULT_LAZY_CONSTRUCT_TIMEOUT);
}

TEST_F(TestSwitcherController, LazyWindowConstruction)
{
  // Setting the timeout to a lower value to speed-up the test
  SwitcherController controller(2);

  EXPECT_EQ(controller.GetConstructTimeout(), 2);

  Utils::WaitForTimeout(controller.GetConstructTimeout()/2);
  ASSERT_FALSE(controller->window_constructed_);

  Utils::WaitUntil(controller.window_constructed_, controller.GetConstructTimeout() + 1);
  EXPECT_TRUE(controller.window_constructed_);
}
#endif

TEST_F(TestSwitcherController, InitiateDetail)
{
  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);
  controller_->InitiateDetail();

  auto const& view = controller_->GetView();
  auto const& model = view->GetModel();
  EXPECT_EQ(controller_->detail_mode(), DetailMode::TAB_NEXT_WINDOW);
  EXPECT_TRUE(model->detail_selection());
}

TEST_F(TestSwitcherController, DisconnectWMSignalsOnDestruction)
{
  auto& color_property = WindowManager::Default().average_color;
  size_t before = color_property.changed.size();
  { Controller dummy; }
  ASSERT_EQ(before, color_property.changed.size());
  color_property.changed.emit(nux::color::RandomColor());
}

TEST_F(TestSwitcherController, InitiateDetailWebapps)
{
  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);

  controller_->Select(3);
  controller_->InitiateDetail();

  auto const& view = controller_->GetView();
  auto const& model = view->GetModel();
  EXPECT_EQ(controller_->detail_mode(), DetailMode::TAB_NEXT_WINDOW);
  EXPECT_FALSE(model->detail_selection());
}

TEST_F(TestSwitcherController, StartDetailMode)
{
  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);
  controller_->InitiateDetail();
  EXPECT_TRUE(controller_->detail());

  auto const& view = controller_->GetView();

  view->switcher_stop_detail.emit();
  EXPECT_FALSE(controller_->detail());

  view->switcher_start_detail.emit();
  EXPECT_TRUE(controller_->detail());
}

TEST_F(TestSwitcherController, StopDetailMode)
{
  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);
  controller_->InitiateDetail();
  EXPECT_TRUE(controller_->detail());

  auto const& view = controller_->GetView();

  view->switcher_stop_detail.emit();
  EXPECT_FALSE(controller_->detail());
}

TEST_F(TestSwitcherController, StartDetailModeMovesNextRows)
{
  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);
  controller_->Select(2);
  controller_->InitiateDetail();

  auto view = controller_->GetView();
  auto model = view->GetModel();
  model->SetRowSizes({2,2});

  view->switcher_start_detail.emit();
  EXPECT_TRUE(controller_->detail());

  view->switcher_start_detail.emit();

  // Grid: Assert we have gone down a row from index 0 -> 2
  //  0, 1,
  //  2, 3
  EXPECT_TRUE(model->HasPrevDetailRow());
  EXPECT_EQ(model->detail_selection_index(), 2);
}

TEST_F(TestSwitcherController, StopDetailModeMovesPrevRows)
{
  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);
  controller_->Select(2);
  controller_->InitiateDetail();

  auto const& view = controller_->GetView();

  view->switcher_start_detail.emit();
  EXPECT_TRUE(controller_->detail());

  auto model = view->GetModel();
  model->SetRowSizes({2,2});

  view->switcher_start_detail.emit();
  view->switcher_stop_detail.emit();

  // Assert we have gone up a row from index 2 -> 0
  //  0, 1,
  //  2, 3
  EXPECT_FALSE(model->HasPrevDetailRow());
  EXPECT_EQ(static_cast<unsigned int>(model->detail_selection_index), 0);

  // Now we are in index 0, stoping detail mode must exit detail mode
  view->switcher_stop_detail.emit();
  EXPECT_FALSE(controller_->detail());
}

TEST_F(TestSwitcherController, ShowSwitcher)
{
  EXPECT_FALSE(controller_->Visible());
  EXPECT_CALL(*mock_window_, ShowWindow(true, _)).Times(AtLeast(1));

  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);
  Utils::WaitUntilMSec([this] { return controller_->Visible(); });
  EXPECT_TRUE(controller_->Visible());
  EXPECT_TRUE(controller_->StartIndex() == 1);
}

TEST_F(TestSwitcherController, ShowSwitcherNoShowDeskop)
{
  EXPECT_CALL(*mock_window_, ShowWindow(true, _)).Times(AtLeast(1));

  controller_->show_desktop_disabled = true;
  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);
  Utils::WaitUntilMSec([this] { return controller_->Visible(); });
  ASSERT_TRUE(controller_->StartIndex() == 0);
  Selection selection = controller_->GetCurrentSelection();
  EXPECT_NE(selection.application_->tooltip_text(), "Show Desktop");

  controller_->Next();
  selection = controller_->GetCurrentSelection();
  EXPECT_NE(selection.application_->tooltip_text(), "Show Desktop");
}

TEST_F(TestSwitcherController, ShowSwitcherNoResults)
{
  controller_->show_desktop_disabled = true;
  std::vector<unity::launcher::AbstractLauncherIcon::Ptr> results;
  EXPECT_CALL(*mock_window_, ShowWindow(true, _)).Times(0);

  controller_->Show(ShowMode::CURRENT_VIEWPORT, SortMode::FOCUS_ORDER, results);
  Utils::WaitForTimeoutMSec(200);
  ASSERT_FALSE(controller_->Visible());
  Selection selection = controller_->GetCurrentSelection();
  EXPECT_FALSE(selection.application_);
}

TEST_F(TestSwitcherController, ShowSwitcherSelectsWindowOfActiveApp)
{
  // Making the first application active
  auto const& first_app = icons_[1];
  first_app->SetQuirk(launcher::AbstractLauncherIcon::Quirk::ACTIVE, true);

  // Raising the priority of the second window of the first app
  auto first_app_windows = first_app->Windows();
  auto first_app_last_window = first_app_windows.back();
  auto standalone = WM->GetWindowByXid(first_app_last_window->window_id());
  standalone->active_number = WM->GetWindowActiveNumber(first_app_windows.front()->window_id()) - 1;

  // Setting the active number of the first window of the second app to max uint
  auto second_app_first_window = icons_[2]->Windows().front();
  standalone = WM->GetWindowByXid(second_app_first_window->window_id());
  standalone->active_number = std::numeric_limits<unsigned>::max();

  controller_->Show(ShowMode::CURRENT_VIEWPORT, SortMode::FOCUS_ORDER, icons_);
  Utils::WaitUntilMSec([this] { return controller_->Visible(); });

  Selection selection = controller_->GetCurrentSelection();
  ASSERT_EQ(selection.application_, first_app);
  EXPECT_EQ(selection.window_, first_app_last_window->window_id());
}

TEST_F(TestSwitcherController, Opacity)
{
  EXPECT_EQ(controller_->Opacity(), 0.0f);

  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);
  tick_source_.tick(TICK_DURATION);

  EXPECT_EQ(controller_->Opacity(), mock_window_->GetOpacity());
}

TEST_F(TestSwitcherController, ShowHideSwitcherFading)
{
  long long global_tick = 0, t;

  EXPECT_CALL(*mock_window_, ShowWindow(true, _)).Times(1);
  {
    InSequence showing;
    EXPECT_CALL(*mock_window_, SetOpacity(Eq(0.0f))).Times(AtLeast(1));

    EXPECT_CALL(*mock_window_, SetOpacity(AllOf(Gt(0.0f), Lt(1.0f))))
      .Times(AtLeast(FADE_DURATION/TICK_DURATION-1));

    EXPECT_CALL(*mock_window_, SetOpacity(Eq(1.0f))).Times(AtLeast(1));
  }

  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);
  ASSERT_EQ(mock_window_->GetOpacity(), 0.0f);

  for (t = global_tick; t < global_tick + FADE_DURATION+1; t += TICK_DURATION)
    tick_source_.tick(t);
  global_tick += t;

  ASSERT_EQ(mock_window_->GetOpacity(), 1.0);
  Mock::VerifyAndClearExpectations(mock_window_.GetPointer());

  {
    InSequence hiding;
    EXPECT_CALL(*mock_window_, SetOpacity(Eq(1.0f))).Times(AtLeast(1));

    EXPECT_CALL(*mock_window_, SetOpacity(AllOf(Lt(1.0f), Gt(0.0f))))
      .Times(AtLeast(FADE_DURATION/TICK_DURATION-1));

    EXPECT_CALL(*mock_window_, SetOpacity(Eq(0.0f))).Times(AtLeast(1));
    EXPECT_CALL(*mock_window_, ShowWindow(false, _)).Times(1);
  }

  controller_->Hide(false);
  ASSERT_EQ(mock_window_->GetOpacity(), 1.0);

  for (t = global_tick; t < global_tick + FADE_DURATION+1; t += TICK_DURATION)
    tick_source_.tick(t);
  global_tick += t;

  EXPECT_EQ(mock_window_->GetOpacity(), 0.0f);
  Mock::VerifyAndClearExpectations(mock_window_.GetPointer());
}

TEST_F(TestSwitcherController, TestRightClickedReceived)
{
  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);

  auto const& view = controller_->GetView();
  auto const& model = view->GetModel();

  ASSERT_FALSE(model->detail_selection());

  view->switcher_mouse_up.emit(-1, 3);
  view->switcher_mouse_down.emit(-1, 3);

  ASSERT_TRUE(model->detail_selection());
}

TEST_F(TestSwitcherController, TestHideRequest)
{
  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);

  auto const& view = controller_->GetView();

  ASSERT_TRUE(controller_->Visible());

  view->hide_request.emit(false);

  ASSERT_FALSE(controller_->Visible());
}
