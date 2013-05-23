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
 * Authored by: Sam Spilsbury <smspillaz@gmail.com>
 */
#ifndef UNITY_MOCK_WINDOW_MANAGER_H
#define UNITY_MOCK_WINDOW_MANAGER_H

#include <gmock/gmock.h>

#include <WindowManager.h>

namespace unity
{
class MockWindowManager : public WindowManager
{
  public:

    MockWindowManager();
    ~MockWindowManager();

    MOCK_CONST_METHOD0(GetActiveWindow, Window());
    MOCK_CONST_METHOD0(GetWindowsInStackingOrder, std::vector<Window>());
    MOCK_CONST_METHOD1(IsWindowDecorated, bool(Window));
    MOCK_CONST_METHOD1(IsWindowMaximized, bool(Window));
    MOCK_CONST_METHOD1(IsWindowOnCurrentDesktop, bool(Window));
    MOCK_CONST_METHOD1(IsWindowObscured, bool(Window));
    MOCK_CONST_METHOD1(IsWindowMapped, bool(Window));
    MOCK_CONST_METHOD1(IsWindowVisible, bool(Window));
    MOCK_CONST_METHOD1(IsWindowOnTop, bool(Window));
    MOCK_CONST_METHOD1(IsWindowClosable, bool(Window));
    MOCK_CONST_METHOD1(IsWindowMinimized, bool(Window));
    MOCK_CONST_METHOD1(IsWindowMinimizable, bool(Window));
    MOCK_CONST_METHOD1(IsWindowMaximizable, bool(Window));
    MOCK_CONST_METHOD1(HasWindowDecorations, bool(Window));

    MOCK_METHOD0(ShowDesktop, void());
    MOCK_CONST_METHOD0(InShowDesktop, bool());

    MOCK_METHOD1(Maximize, void(Window));
    MOCK_METHOD1(Restore, void(Window));
    MOCK_METHOD3(RestoreAt, void(Window, int, int));
    MOCK_METHOD1(Minimize, void(Window));
    MOCK_METHOD1(UnMinimize, void(Window));
    MOCK_METHOD1(Close, void(Window));
    MOCK_METHOD1(Activate, void(Window));
    MOCK_METHOD1(Raise, void(Window));
    MOCK_METHOD1(Lower, void(Window));
    MOCK_METHOD2(RestackBelow, void(Window, Window));

    MOCK_METHOD0(TerminateScale, void());
    MOCK_CONST_METHOD0(IsScaleActive, bool());
    MOCK_CONST_METHOD0(IsScaleActiveForGroup, bool());
    MOCK_METHOD0(InitiateExpo, void());
    MOCK_METHOD0(TerminateExpo, void());
    MOCK_CONST_METHOD0(IsExpoActive, bool());
    MOCK_CONST_METHOD0(IsWallActive, bool());

    MOCK_METHOD4(FocusWindowGroup, void(std::vector<Window> const&,
                                        FocusVisibility, int,
                                        bool));
    MOCK_METHOD3(ScaleWindowGroup, bool(std::vector<Window> const&, int, bool));

    MOCK_CONST_METHOD0(IsScreenGrabbed, bool());
    MOCK_CONST_METHOD0(IsViewPortSwitchStarted, bool());

    MOCK_METHOD2(MoveResizeWindow, void(Window, nux::Geometry));
    MOCK_METHOD3(StartMove, void(Window, int, int));

    MOCK_CONST_METHOD1(GetWindowMonitor, int(Window));
    MOCK_CONST_METHOD1(GetWindowGeometry, nux::Geometry(Window));
    MOCK_CONST_METHOD1(GetWindowSavedGeometry, nux::Geometry(Window));
    MOCK_CONST_METHOD2(GetWindowDecorationSize, nux::Size(Window, WindowManager::Edge));
    MOCK_CONST_METHOD0(GetScreenGeometry, nux::Geometry());
    MOCK_CONST_METHOD1(GetWorkAreaGeometry, nux::Geometry(Window));

    MOCK_CONST_METHOD1(GetWindowActiveNumber, uint64_t(Window));

    MOCK_METHOD2(SetWindowIconGeometry, void(Window, nux::Geometry const&));

    MOCK_METHOD3(CheckWindowIntersections, void(nux::Geometry const&, bool&, bool&));

    MOCK_CONST_METHOD0(WorkspaceCount, int());

    MOCK_METHOD0(SaveInputFocus, bool());
    MOCK_METHOD0(RestoreInputFocus, bool());

    MOCK_CONST_METHOD1(GetWindowName, std::string(Window));

    MOCK_METHOD1(AddProperties, void(GVariantBuilder*));
};
}

#endif // UNITY_MOCK_WINDOW_MANAGER_H
