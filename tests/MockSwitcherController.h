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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 *
 */
#ifndef _UNITY_MOCK_SWITCHER_CONTROLLER_H
#define _UNITY_MOCK_SWITCHER_CONTROLLER_H

#include <memory>

#include <gmock/gmock.h>

#include <SwitcherController.h>
#include <SwitcherView.h>
#include <AbstractLauncherIcon.h>

namespace unity
{
namespace switcher
{
class MockSwitcherController : public Controller::Impl
{
public:
  typedef std::shared_ptr <MockSwitcherController> Ptr;

  MOCK_METHOD3(Show, void(ShowMode, SortMode, std::vector<launcher::AbstractLauncherIcon::Ptr>));
  MOCK_METHOD1(Hide, void(bool));
  MOCK_METHOD0(Visible, bool());
  MOCK_METHOD0(Next, void());
  MOCK_METHOD0(Prev, void());
  MOCK_METHOD1(Select, void(int));
  MOCK_METHOD0(GetView, unity::switcher::SwitcherView * ());
  MOCK_CONST_METHOD1(CanShowSwitcher, bool(const std::vector<launcher::AbstractLauncherIcon::Ptr> &));
  MOCK_METHOD0(NextDetail, void());
  MOCK_METHOD0(PrevDetail, void());
  MOCK_METHOD2(SetDetail, void(bool, unsigned int));
  MOCK_METHOD0(SelectFirstItem, void());
  MOCK_METHOD2(SetWorkspace, void(nux::Geometry, int));
  MOCK_METHOD0(ExternalRenderTargets, unity::ui::LayoutWindow::Vector ());
  MOCK_CONST_METHOD0(GetSwitcherInputWindowId, guint());
  MOCK_CONST_METHOD0(IsShowDesktopDisabled, bool());
  MOCK_METHOD1(SetShowDesktopDisabled, void(bool));
  MOCK_CONST_METHOD0(StartIndex, int());
};
}
}

#endif
