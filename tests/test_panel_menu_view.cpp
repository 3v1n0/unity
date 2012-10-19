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
 */

#include <gmock/gmock.h>

#include <Nux/Nux.h>
#include "PanelMenuView.h"
#include "PanelStyle.h"
#include "UnitySettings.h"
#include "UBusMessages.h"
// #include "StandaloneWindowManager.h"
#include "test_utils.h"

using namespace testing;

namespace unity
{

class MockPanelMenuView : public PanelMenuView
{
  protected:
    virtual std::string GetActiveViewName(bool use_appname) const
    {
      return "<>'";
    }
};

class MockWindowManager : public WindowManager
{
public:
  guint32 GetActiveWindow() const { return 0; }
  unsigned long long GetWindowActiveNumber (guint32 xid) const { return 0; }
  bool IsScreenGrabbed() const { return false; }
  bool IsViewPortSwitchStarted() const { return false; }
  void ShowDesktop() { }
  bool InShowDesktop() const { return false; }
  bool IsWindowMaximized(guint32 xid) const { return false; }
  bool IsWindowDecorated(guint32 xid) { return true; }
  bool IsWindowOnCurrentDesktop(guint32 xid) const { return true; }
  bool IsWindowObscured(guint32 xid) const { return false; }
  bool IsWindowMapped(guint32 xid) const { return true; }
  bool IsWindowVisible(guint32 xid) const { return true; }
  bool IsWindowOnTop(guint32 xid) const { return false; }
  bool IsWindowClosable(guint32 xid) const { return true; }
  bool IsWindowMinimizable(guint32 xid) const { return true; }
  bool IsWindowMaximizable(guint32 xid) const { return true; }
  void Restore(guint32 xid) { }
  void RestoreAt(guint32 xid, int x, int y) { }
  void Minimize(guint32 xid) { }
  void Close(guint32 xid) { }
  void Activate(guint32 xid) { }
  void Raise(guint32 xid) { }
  void Lower(guint32 xid) { }
  void FocusWindowGroup(std::vector<Window> windows, FocusVisibility, int monitor, bool only_top_win) { }
  bool ScaleWindowGroup(std::vector<Window> windows, int state, bool force) { return false; }
  int GetWindowMonitor(guint32 xid) const { return -1; }
  void SetWindowIconGeometry(Window window, nux::Geometry const& geo) { }
  int WorkspaceCount () const { return 1; }
  void TerminateScale() { }
  void SetScaleActive(bool scale_active) { _scale_active = scale_active; }
  bool IsScaleActive() const { return _scale_active; }
  void SetScaleActiveForGroup(bool scale_active_group) { _scale_active_group = scale_active_group; }
  bool IsScaleActiveForGroup() const { return _scale_active_group; }
  void InitiateExpo() { }
  void TerminateExpo() { }
  bool IsExpoActive() const { return false; }
  bool IsWallActive() const { return false; }
  void MoveResizeWindow(guint32 xid, nux::Geometry geometry) { }
  bool saveInputFocus() { return false; }
  bool restoreInputFocus() { return false; }
  void AddProperties(GVariantBuilder* builder) {}
  std::string GetWindowName(guint32 xid) const { return ""; }

  nux::Geometry GetWindowGeometry(guint xid) const
  {
    int width = (guint32)xid >> 16;
    int height = (guint32)xid & 0x0000FFFF;
    return nux::Geometry(0, 0, width, height);
  }

  nux::Geometry GetWindowSavedGeometry(guint xid) const
  {
    return nux::Geometry(0, 0, 1, 1);
  }

  nux::Geometry GetScreenGeometry() const
  {
    return nux::Geometry(0, 0, 1, 1);
  }

  nux::Geometry GetWorkAreaGeometry(guint32 xid) const
  {
    return nux::Geometry(0, 0, 1, 1);
  }

  void CheckWindowIntersections (nux::Geometry const& region, bool &active, bool &any)
  {
    active = false;
    any = false;
  }

  bool _scale_active = false;
  bool _scale_active_group = false;
};

struct TestPanelMenuView : public testing::Test
{
  virtual void SetUp()
  {
  }

  void ProcessUBusMessages()
  {
    bool expired = false;
    glib::Idle idle([&] { expired = true; return false; },
                    glib::Source::Priority::LOW);
    Utils::WaitUntil(expired);
  }

  std::string panelTitle() const
  {
    const PanelMenuView *panel = &panelMenuView;
    return panel->GetCurrentTitle();
  }

protected:
  // The order is important, i.e. PanelMenuView needs
  // panel::Style that needs Settings
  MockWindowManager windowManager;
  Settings settings;
  panel::Style panelStyle;
  MockPanelMenuView panelMenuView;
};

TEST_F(TestPanelMenuView, Escaping)
{
  WindowManager::SetDefault(&windowManager);
  static const char *escapedText = "Panel d&amp;Inici";
  EXPECT_TRUE(panelTitle().empty());

  UBusManager ubus;
  ubus.SendMessage(UBUS_LAUNCHER_START_KEY_NAV, NULL);
  ubus.SendMessage(UBUS_LAUNCHER_SELECTION_CHANGED,
                   g_variant_new_string(escapedText));
  ProcessUBusMessages();

  EXPECT_EQ(panelTitle(), escapedText);

  ubus.SendMessage(UBUS_LAUNCHER_END_KEY_NAV, NULL);
  ProcessUBusMessages();

  // Change the wm to trick PanelMenuView::RefreshTitle to call GetActiveViewName
  windowManager.SetScaleActive(true);
  windowManager.SetScaleActiveForGroup(true);

  EXPECT_EQ(panelTitle(), "&lt;&gt;&apos;");
}

}
