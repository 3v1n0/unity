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

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>
#include <libbamf/libbamf.h>

#include "PanelIndicatorsView.h"
#include "StaticCairoText.h"
#include "WindowButtons.h"
#include "PanelTitlebarGrabAreaView.h"
#include "PluginAdapter.h"
#include "Animator.h"
#include "UBusWrapper.h"

namespace unity
{

class PanelMenuView : public PanelIndicatorsView
{
public:
  PanelMenuView();
  ~PanelMenuView();

  void SetMenuShowTimings(int fadein, int fadeout, int discovery,
                          int discovery_fadein, int discovery_fadeout);

  void SetMousePosition(int x, int y);
  void SetMonitor(int monitor);

  Window GetTopWindow() const;
  Window GetMaximizedWindow() const;
  bool GetControlsActive() const;

  void NotifyAllMenusClosed();

  virtual void AddIndicator(indicator::Indicator::Ptr const& indicator);

  virtual void OverlayShown();
  virtual void OverlayHidden();

protected:
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void PreLayoutManagement();
  virtual nux::Area* FindAreaUnderMouse(const nux::Point& mouse_position,
                                        nux::NuxEventType event_type);
  virtual void OnEntryAdded(indicator::Entry::Ptr const& entry);

private:
  void OnActiveChanged(PanelIndicatorEntryView* view, bool is_active);
  void OnViewOpened(BamfMatcher* matcher, BamfView* view);
  void OnViewClosed(BamfMatcher* matcher, BamfView* view);
  void OnApplicationClosed(BamfApplication* app);
  void OnActiveWindowChanged(BamfMatcher* matcher, BamfView* old_view, BamfView* new_view);
  void OnActiveAppChanged(BamfMatcher* matcher, BamfApplication* old_app, BamfApplication* new_app);
  void OnNameChanged(BamfView* bamf_view, gchar* new_name, gchar* old_name);

  void OnSpreadInitiate();
  void OnSpreadTerminate();
  void OnExpoInitiate();
  void OnExpoTerminate();
  void OnWindowMinimized(guint32 xid);
  void OnWindowUnminimized(guint32 xid);
  void OnWindowUnmapped(guint32 xid);
  void OnWindowMapped(guint32 xid);
  void OnWindowMaximized(guint32 xid);
  void OnWindowRestored(guint32 xid);
  void OnWindowMoved(guint32 xid);
  void OnWindowDecorated(guint32 xid);
  void OnWindowUndecorated(guint32 xid);

  void OnMaximizedActivate(int x, int y);
  void OnMaximizedRestore(int x, int y);
  void OnMaximizedLower(int x, int y);
  void OnMaximizedGrabStart(int x, int y);
  void OnMaximizedGrabMove(int x, int y);
  void OnMaximizedGrabEnd(int x, int y);

  void FullRedraw();
  void Refresh(bool force = false);
  void DrawTitle(cairo_t *cr_real, nux::Geometry const& geo, std::string const& label) const;

  void OnPanelViewMouseEnter(int x, int y, unsigned long mouse_button_state, unsigned long special_keys_state);
  void OnPanelViewMouseLeave(int x, int y, unsigned long mouse_button_state, unsigned long special_keys_state);
  void OnPanelViewMouseMove(int x, int y, int dx, int dy, unsigned long mouse_button_state, unsigned long special_keys_state);

  BamfWindow* GetBamfWindowForXid(Window xid) const;

  std::string GetActiveViewName(bool use_appname = false) const;

  void OnSwitcherShown(GVariant* data);
  void OnSwitcherSelectionChanged(GVariant* data);
  void OnLauncherKeyNavStarted(GVariant* data);
  void OnLauncherKeyNavEnded(GVariant* data);
  void OnLauncherSelectionChanged(GVariant* data);

  void UpdateShowNow(bool ignore);

  static gboolean UpdateActiveWindowPosition(PanelMenuView* self);
  static gboolean UpdateShowNowWithDelay(PanelMenuView* self);
  static gboolean OnNewAppShow(PanelMenuView* self);
  static gboolean OnNewAppHide(PanelMenuView* self);

  bool IsValidWindow(Window xid) const;
  bool IsWindowUnderOurControl(Window xid) const;

  bool DrawMenus() const;
  bool DrawWindowButtons() const;

  void OnFadeInChanged(double);
  void OnFadeOutChanged(double);

  glib::Object<BamfMatcher> _matcher;

  nux::TextureLayer* _title_layer;
  nux::HLayout* _menu_layout;
  nux::ObjectPtr<nux::BaseTexture> _title_texture;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> _gradient_texture;

  bool _is_inside;
  bool _is_grabbed;
  bool _is_maximized;

  PanelIndicatorEntryView* _last_active_view;
  WindowButtons* _window_buttons;
  PanelTitlebarGrabArea* _titlebar_grab_area;
  glib::Object<BamfApplication> _new_application;

  std::map<Window, bool> _decor_map;
  std::set<Window> _maximized_set;
  std::list<glib::Object<BamfApplication>> _new_apps;
  std::string _panel_title;
  nux::Geometry _last_geo;

  bool _overlay_showing;
  bool _switcher_showing;
  bool _launcher_keynav;
  bool _show_now_activated;
  bool _we_control_active;
  bool _new_app_menu_shown;

  int _monitor;
  Window _active_xid;

  guint32 _active_moved_id;
  guint32 _update_show_now_id;
  guint32 _new_app_show_id;
  guint32 _new_app_hide_id;
  nux::Geometry _monitor_geo;

  glib::Signal<void, BamfMatcher*, BamfView*> _view_opened_signal;
  glib::Signal<void, BamfMatcher*, BamfView*> _view_closed_signal;
  glib::Signal<void, BamfMatcher*, BamfView*, BamfView*> _active_win_changed_signal;
  glib::Signal<void, BamfMatcher*, BamfApplication*, BamfApplication*> _active_app_changed_signal;
  glib::Signal<void, BamfView*, gchar*, gchar*> _view_name_changed_signal;
  sigc::connection _style_changed_connection;

  UBusManager _ubus_manager;

  int _menus_fadein;
  int _menus_fadeout;
  int _menus_discovery;
  int _menus_discovery_fadein;
  int _menus_discovery_fadeout;

  Animator _fade_in_animator;
  Animator _fade_out_animator;

  const std::string _desktop_name;
};

}

#endif
