// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
 */

#ifndef PANEL_MENU_VIEW_H
#define PANEL_MENU_VIEW_H

#include <Nux/View.h>
#include <map>
#include <set>

#include "PanelIndicatorsView.h"
#include "StaticCairoText.h"
#include "WindowButtons.h"
#include "PanelTitlebarGrabAreaView.h"
#include "PluginAdapter.h"
#include "Animator.h"

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>
#include <libbamf/libbamf.h>

namespace unity
{

class PanelMenuView : public PanelIndicatorsView
{
public:
  // This contains all the menubar logic for the Panel. Mainly it contains
  // the following states:
  // 1. Unmaximized window + no mouse hover
  // 2. Unmaximized window + mouse hover
  // 3. Unmaximized window + active menu (Alt+F/arrow key nav)
  // 4. Maximized window + no mouse hover
  // 5. Maximized window + mouse hover
  // 6. Maximized window + active menu
  //
  // It also deals with undecorating maximized windows (and redecorating them
  // on unmaximize)

  PanelMenuView(int padding = 6);
  ~PanelMenuView();

  void SetMenuShowTimings(int fadein, int fadeout, int discovery,
                          int discovery_fadein, int discovery_fadeout);

  void FullRedraw();

  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual long PostLayoutManagement(long LayoutResult);

  void SetMousePosition(int x, int y);

  void OnActiveChanged(PanelIndicatorEntryView* view, bool is_active);
  void OnViewOpened(BamfMatcher* matcher, BamfView* view);
  void OnViewClosed(BamfMatcher* matcher, BamfView* view);
  void OnActiveWindowChanged(BamfMatcher* matcher, BamfView* old_view, BamfView* new_view);
  void OnActiveAppChanged(BamfMatcher* matcher, BamfApplication* old_app, BamfApplication* new_app);
  void OnNameChanged(BamfView* bamf_view, gchar* new_name, gchar* old_name);

  void OnSpreadInitiate();
  void OnSpreadTerminate();
  void OnWindowMinimized(guint32 xid);
  void OnWindowUnminimized(guint32 xid);
  void OnWindowUnmapped(guint32 xid);
  void OnWindowMapped(guint32 xid);
  void OnWindowMaximized(guint32 xid);
  void OnWindowRestored(guint32 xid);
  void OnWindowMoved(guint32 xid);
  void OnWindowDecorated(guint32 xid);
  void OnWindowUndecorated(guint32 xid);

  guint32 GetMaximizedWindow();

  void OnMaximizedGrabStart(int, int, unsigned long, unsigned long);
  void OnMaximizedGrabMove(int, int, int, int, unsigned long, unsigned long);
  void OnMaximizedGrabEnd(int, int, unsigned long, unsigned long);
  void OnMouseDoubleClicked(int, int, unsigned long, unsigned long);
  void OnMouseClicked(int, int, unsigned long, unsigned long);
  void OnMouseMiddleClicked(int, int, unsigned long, unsigned long);

  void Refresh();
  void AllMenusClosed();

  void OnCloseClicked();
  void OnMinimizeClicked();
  void OnRestoreClicked();
  void SetMonitor(int monitor);
  bool GetControlsActive();

  bool HasOurWindowFocused();

protected:
  std::string GetName() const;
  void          AddProperties(GVariantBuilder* builder);

  virtual nux::Area* FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type);
  void OnPanelViewMouseEnter(int x, int y, unsigned long mouse_button_state, unsigned long special_keys_state);
  void OnPanelViewMouseLeave(int x, int y, unsigned long mouse_button_state, unsigned long special_keys_state);
  void OnPanelViewMouseMove(int x, int y, int dx, int dy, unsigned long mouse_button_state, unsigned long special_keys_state);
  virtual void OnEntryAdded(unity::indicator::Entry::Ptr const& entry);

private:
  std::string GetActiveViewName();
  static void OnPlaceViewShown(GVariant* data, PanelMenuView* self);
  static void OnPlaceViewHidden(GVariant* data, PanelMenuView* self);
  static void OnSwitcherShown(GVariant* data, PanelMenuView* self);
  static void OnSwitcherSelectionChanged(GVariant* data, PanelMenuView* self);

  void UpdateShowNow(bool ignore);

  static gboolean UpdateActiveWindowPosition(PanelMenuView* self);
  static gboolean UpdateShowNowWithDelay(PanelMenuView* self);
  static gboolean OnNewAppShow(PanelMenuView* self);
  static gboolean OnNewAppHide(PanelMenuView* self);

  void DrawText(cairo_t *cr_real, int x, int y, int width, int height,
                std::string const& font_desc, std::string const& label,
                int increase_size=0);

  bool DrawMenus();
  bool DrawWindowButtons();

  void OnFadeInChanged(double);
  void OnFadeOutChanged(double);

private:
  glib::Object<BamfMatcher> _matcher;

  nux::TextureLayer*       _title_layer;
  nux::HLayout*            _menu_layout;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> _gradient_texture;

  bool _is_inside;
  bool _is_grabbed;
  bool _is_maximized;
  bool _is_own_window;
  PanelIndicatorEntryView* _last_active_view;
  glib::Object<BamfApplication> _new_application;

  WindowButtons* _window_buttons;
  PanelTitlebarGrabArea* _panel_titlebar_grab_area;

  std::map<guint32, bool> _decor_map;
  std::set<guint32> _maximized_set;
  std::list<glib::Object<BamfApplication>> _new_apps;
  std::string _panel_title;

  int _padding;
  int _last_width;
  int _last_height;

  bool _places_showing;
  bool _switcher_showing;
  bool _show_now_activated;
  bool _we_control_active;
  bool _new_app_menu_shown;

  int  _monitor;
  guint32 _active_xid;
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

  std::vector<unsigned int> _ubus_interests;

  int _menus_fadein;
  int _menus_fadeout;
  int _menus_discovery;
  int _menus_discovery_fadein;
  int _menus_discovery_fadeout;

  Animator _fade_in_animator;
  Animator _fade_out_animator;
};

}

#endif
