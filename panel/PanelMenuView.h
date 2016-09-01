// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *              Marco Trevisan <3v1n0@ubuntu.com>
 */

#ifndef PANEL_MENU_VIEW_H
#define PANEL_MENU_VIEW_H

#include <NuxCore/Animation.h>
#include <UnityCore/GLibSignal.h>

#include "PanelIndicatorsView.h"
#include "PanelTitlebarGrabAreaView.h"
#include "unity-shared/ApplicationManager.h"
#include "unity-shared/MenuManager.h"
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/WindowButtons.h"
#include "unity-shared/UBusWrapper.h"

namespace unity
{
namespace panel
{

class PanelMenuView : public PanelIndicatorsView
{
public:
  PanelMenuView(menu::Manager::Ptr const&);
  ~PanelMenuView();

  nux::Property<Window> active_window;
  nux::Property<Window> maximized_window;
  nux::Property<bool> focused;

  void SetMousePosition(int x, int y);
  void SetMonitor(int monitor) override;

  Window GetTopWindow() const;
  bool GetControlsActive() const;
  bool HasMenus() const;
  bool HasKeyActivableMenus() const;

  void NotifyAllMenusClosed();

  virtual void AddIndicator(indicator::Indicator::Ptr const& indicator);

  virtual void OverlayShown();
  virtual void OverlayHidden();

protected:
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void PreLayoutManagement();
  virtual nux::Area* FindAreaUnderMouse(const nux::Point& mouse_position,
                                        nux::NuxEventType event_type);
  virtual void OnEntryAdded(indicator::Entry::Ptr const& entry);
  virtual std::string GetActiveViewName(bool use_appname = false) const;
  virtual std::string GetMaximizedViewName(bool use_appname = false) const;

private:
  friend class TestPanelMenuView;

  void SetupPanelMenuViewSignals();
  void SetupWindowButtons();
  void SetupLayout();
  void SetupTitlebarGrabArea();
  void SetupWindowManagerSignals();
  void SetupUBusManagerInterests();

  void OnActiveChanged(PanelIndicatorEntryView* view, bool is_active);
  void OnEntryViewAdded(PanelIndicatorEntryView* view);
  void OnApplicationStarted(ApplicationPtr const&);
  void OnApplicationClosed(ApplicationPtr const&);
  void OnWindowOpened(ApplicationWindowPtr const&);
  void OnWindowClosed(ApplicationWindowPtr const&);
  void OnActiveWindowChanged(ApplicationWindowPtr const&);
  void OnActiveAppChanged(ApplicationPtr const&);
  void OnStyleChanged();
  void OnLIMChanged(bool);
  void OnAlwaysShowMenusChanged(bool);

  void OnSpreadInitiate();
  void OnSpreadTerminate();
  void OnWindowMinimized(Window xid);
  void OnWindowUnminimized(Window xid);
  void OnWindowUnmapped(Window xid);
  void OnWindowMapped(Window xid);
  void OnWindowMaximized(Window xid);
  void OnWindowRestored(Window xid);
  void OnWindowUnFullscreen(Window xid);
  void OnWindowMoved(Window xid);
  void OnShowDesktopChanged();

  void OnMaximizedActivate(int x, int y);
  void OnMaximizedDoubleClicked(int x, int y);
  void OnMaximizedMiddleClicked(int x, int y);
  void OnMaximizedRightClicked(int x, int y);
  void OnMaximizedGrabStart(int x, int y);
  void OnMaximizedGrabMove(int x, int y);
  void OnMaximizedGrabEnd(int x, int y);
  void MaximizedWindowWMAction(int x, int y, unsigned button);

  void FullRedraw();
  std::string GetCurrentTitle() const;
  bool Refresh(bool force = false);
  void RefreshAndRedraw();

  void UpdateTitleTexture(nux::Geometry const&, std::string const& label);
  void UpdateLastGeometry(nux::Geometry const& geo);
  void UpdateTitleGradientTexture();
  void UpdateMaximizedWindow();

  void OnPanelViewMouseEnter(int x, int y, unsigned long mouse_button_state, unsigned long special_keys_state);
  void OnPanelViewMouseLeave(int x, int y, unsigned long mouse_button_state, unsigned long special_keys_state);

  void OnSwitcherShown(GVariant* data);
  void OnLauncherKeyNavStarted(GVariant* data);
  void OnLauncherKeyNavEnded(GVariant* data);
  void OnLauncherSelectionChanged(GVariant* data);

  void UpdateTargetWindowItems();
  void UpdateShowNow(bool ignore);
  bool CheckMouseInside();

  bool UpdateActiveWindowPosition();
  bool UpdateShowNowWithDelay();
  bool OnNewAppShow();
  bool OnNewAppHide();

  bool IsValidWindow(Window xid) const;
  bool IsWindowUnderOurControl(Window xid) const;

  bool ShouldDrawMenus() const;
  bool ShouldDrawButtons() const;
  bool ShouldDrawFadingTitle() const;
  bool HasVisibleMenus() const;

  double GetTitleOpacity() const;

  void StartFadeIn(int duration = -1);
  void StartFadeOut(int duration = -1);
  void OnFadeAnimatorUpdated(double opacity);

  void ActivateIntegratedMenus(nux::Point const&);

  menu::Manager::Ptr menu_manager_;

  nux::TextureLayer* title_layer_;
  nux::ObjectPtr<WindowButtons> window_buttons_;
  nux::ObjectPtr<PanelTitlebarGrabArea> titlebar_grab_area_;
  nux::ObjectPtr<nux::BaseTexture> title_texture_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> gradient_texture_;

  bool is_inside_;
  bool is_grabbed_;
  bool is_maximized_;
  bool is_desktop_focused_;

  PanelIndicatorEntryView* last_active_view_;
  std::deque<Window> maximized_wins_;
  ApplicationPtr new_application_;
  std::list<ApplicationPtr> new_apps_;
  std::string panel_title_;
  nux::Geometry last_geo_;
  nux::Geometry title_geo_;

  bool switcher_showing_;
  bool spread_showing_;
  bool launcher_keynav_;
  bool show_now_activated_;
  bool we_control_active_;
  bool new_app_menu_shown_;
  bool ignore_menu_visibility_;
  bool integrated_menus_;
  bool always_show_menus_;

  nux::Geometry monitor_geo_;
  const std::string desktop_name_;

  connection::Wrapper app_name_changed_conn_;
  connection::Wrapper win_name_changed_conn_;

  UBusManager ubus_manager_;
  glib::SourceManager sources_;

  nux::animation::AnimateValue<double> opacity_animator_;
};

} // namespace panel
} // namespace unity

#endif
