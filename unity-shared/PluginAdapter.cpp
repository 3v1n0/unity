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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#include <glib.h>
#include <gdk/gdkx.h>
#include "UScreen.h"
#include "UnitySettings.h"
#include "DecorationStyle.h"
#include "PluginAdapter.h"
#include "CompizUtils.h"
#include "MultiMonitor.h"

#include <NuxGraphics/XInputWindow.h>
#include <NuxCore/Logger.h>

namespace unity
{
DECLARE_LOGGER(logger, "unity.wm.compiz");
namespace
{
const int THRESHOLD_HEIGHT = 600;
const int THRESHOLD_WIDTH = 1024;

std::shared_ptr<PluginAdapter> global_instance;
}

#define MAXIMIZABLE (CompWindowActionMaximizeHorzMask & CompWindowActionMaximizeVertMask & CompWindowActionResizeMask)

WindowManagerPtr create_window_manager()
{
  return global_instance;
}

/* static */
PluginAdapter& PluginAdapter::Default()
{
  // Better hope that initialize has been called first.
  return *global_instance;
}

/* static */
PluginAdapter& PluginAdapter::Initialize(CompScreen* screen)
{
  if (!global_instance)
  {
    global_instance.reset(new PluginAdapter(screen));
  }
  else
  {
    LOG_ERROR(logger) << "Already Initialized!";
  }

  return *global_instance;
}

PluginAdapter::PluginAdapter(CompScreen* screen)
  : bias_active_to_viewport(false)
  , m_Screen(screen)
  , _scale_screen(ScaleScreen::get(screen))
  , _coverage_area_before_automaximize(0.75)
  , _spread_state(false)
  , _spread_requested_state(false)
  , _spread_windows_state(false)
  , _expo_state(false)
  , _vp_switch_started(false)
  , _in_show_desktop(false)
  , _grab_show_action(nullptr)
  , _grab_hide_action(nullptr)
  , _grab_toggle_action(nullptr)
  , _last_focused_window(nullptr)
{}

PluginAdapter::~PluginAdapter()
{}

/* A No-op for now, but could be useful later */
void PluginAdapter::OnScreenGrabbed()
{
  screen_grabbed.emit();

  if (!_spread_state && screen->grabExist("scale"))
  {
    _spread_state = true;
    _spread_requested_state = true;
    initiate_spread.emit();
  }

  if (!_expo_state && screen->grabExist("expo"))
  {
    _expo_state = true;
    initiate_expo.emit();
  }
}

void PluginAdapter::OnScreenUngrabbed()
{
  if (_spread_state && !screen->grabExist("scale"))
  {
    _spread_state = false;
    _spread_requested_state = false;
    _spread_windows_state = false;
    terminate_spread.emit();
  }

  if (_expo_state && !screen->grabExist("expo"))
  {
    _expo_state = false;
    terminate_expo.emit();
  }

  screen_ungrabbed.emit();
}

void PluginAdapter::NotifyResized(CompWindow* window, int x, int y, int w, int h)
{
  window_resized.emit(window->id());
}

void PluginAdapter::NotifyMoved(CompWindow* window, int x, int y)
{
  window_moved.emit(window->id());
}

void PluginAdapter::NotifyStateChange(CompWindow* window, unsigned int state, unsigned int last_state)
{
  if (!((last_state & MAXIMIZE_STATE) == MAXIMIZE_STATE)
      && ((state & MAXIMIZE_STATE) == MAXIMIZE_STATE))
  {
    window_maximized.emit(window->id());
  }
  else if (((last_state & MAXIMIZE_STATE) == MAXIMIZE_STATE)
           && !((state & MAXIMIZE_STATE) == MAXIMIZE_STATE))
  {
    window_restored.emit(window->id());
  }

  if ((state & CompWindowStateFullscreenMask) == CompWindowStateFullscreenMask)
  {
    window_fullscreen.emit(window->id());
  }
  else if (((last_state & CompWindowStateFullscreenMask) == CompWindowStateFullscreenMask) &&
           !((state & CompWindowStateFullscreenMask) == CompWindowStateFullscreenMask))
  {
    window_unfullscreen.emit(window->id());
  }
}

void PluginAdapter::Notify(CompWindow* window, CompWindowNotify notify)
{
  switch (notify)
  {
    case CompWindowNotifyMinimize:
      window_minimized.emit(window->id());
      break;
    case CompWindowNotifyUnminimize:
      window_unminimized.emit(window->id());
      break;
    case CompWindowNotifyShade:
      window_shaded.emit(window->id());
      break;
    case CompWindowNotifyUnshade:
      window_unshaded.emit(window->id());
      break;
    case CompWindowNotifyHide:
      window_hidden.emit(window->id());
      break;
    case CompWindowNotifyShow:
      window_shown.emit(window->id());
      break;
    case CompWindowNotifyMap:
      window_mapped.emit(window->id());
      break;
    case CompWindowNotifyUnmap:
      window_unmapped.emit(window->id());
      break;
    case CompWindowNotifyFocusChange:
      window_focus_changed.emit(window->id());
      break;
    default:
      break;
  }
}

void PluginAdapter::NotifyCompizEvent(const char* plugin,
                                      const char* event,
                                      CompOption::Vector& option)
{
  if (g_strcmp0(event, "start_viewport_switch") == 0)
  {
    _vp_switch_started = true;
    screen_viewport_switch_started.emit();
  }
  else if (g_strcmp0(event, "end_viewport_switch") == 0)
  {
    UpdateShowDesktopState();

    _vp_switch_started = false;
    screen_viewport_switch_ended.emit();
  }
  else if (g_strcmp0(plugin, "scale") == 0 && g_strcmp0(event, "activate") == 0)
  {
    bool new_state = CompOption::getBoolOptionNamed(option, "active");

    if (_spread_state != new_state)
    {
      _spread_state = new_state;
      _spread_requested_state = new_state;
      _spread_state ? initiate_spread.emit() : terminate_spread.emit();

      if (!_spread_state)
        _spread_windows_state = false;
    }
    else if (_spread_state && new_state)
    {
      // If the scale plugin is activated again while is already grabbing the screen
      // it means that is switching the view (i.e. switching from a spread application
      // to another), so we need to notify our clients that it has really terminated
      // and initiated again.

      bool old_windows_state = _spread_windows_state;
      _spread_state = false;
      _spread_requested_state = false;
      _spread_windows_state = false;
      terminate_spread.emit();

      _spread_state = true;
      _spread_requested_state = true;
      _spread_windows_state = old_windows_state;
      initiate_spread.emit();
    }
  }
}

void MultiActionList::AddNewAction(std::string const& name, CompAction* a, bool primary)
{
  actions_[name] = a;

  if (primary)
    primary_action_ = a;
}

void MultiActionList::RemoveAction(std::string const& name)
{
  actions_.erase(name);
}

CompAction* MultiActionList::GetAction(std::string const& name) const
{
  auto it = actions_.find(name);

  if (it == actions_.end())
    return nullptr;

  return it->second;
}

bool MultiActionList::HasPrimary() const
{
  return bool(primary_action_);
}

void MultiActionList::Initiate(std::string const& name, CompOption::Vector const& extra_args, int state) const
{
  if (name.empty())
    return;

  CompAction* action = GetAction(name);

  if (!action)
    return;

  CompOption::Vector argument(1);
  argument[0].setName("root", CompOption::TypeInt);
  argument[0].value().set((int) screen->root());

  for (CompOption const& arg : extra_args)
    argument.push_back(arg);

  /* Initiate the selected action with the arguments */
  if (CompAction::CallBack const& initiate_cb = primary_action_->initiate())
    initiate_cb(action, state, argument);
}

void MultiActionList::InitiateAll(CompOption::Vector const& extra_args, int state) const
{
  if (actions_.empty())
    return;

  std::string action_name;

  if (primary_action_)
  {
    for (auto const& it : actions_)
    {
      if (it.second == primary_action_)
      {
        action_name = it.first;
        break;
      }
    }
  }
  else
  {
    action_name = actions_.begin()->first;
  }

  Initiate(action_name, extra_args, state);
}

void MultiActionList::TerminateAll(CompOption::Vector const& extra_args) const
{
  if (actions_.empty())
    return;

  CompOption::Vector argument(1);
  argument[0].setName("root", CompOption::TypeInt);
  argument[0].value().set((int) screen->root());

  for (CompOption const& a : extra_args)
    argument.push_back(a);

  if (primary_action_)
  {
    if (CompAction::CallBack const& terminate_cb = primary_action_->terminate())
    {
      terminate_cb(primary_action_, CompAction::StateCancel, argument);
      return;
    }
  }

  for (auto const& it : actions_)
  {
    CompAction* action = it.second;

    if (action->state() & (CompAction::StateTermKey |
                           CompAction::StateTermButton |
                           CompAction::StateTermEdge |
                           CompAction::StateTermEdgeDnd))
    {
      if (CompAction::CallBack const& terminate_cb = primary_action_->terminate())
        terminate_cb(action, 0, argument);
    }
  }
}

uint64_t PluginAdapter::GetWindowActiveNumber(Window window_id) const
{
  CompWindow* window = m_Screen->findWindow(window_id);

  if (window)
  {
    // result is actually an unsigned int (32 bits)
    uint64_t result = window->activeNum ();
    if (bias_active_to_viewport() && window->defaultViewport() == m_Screen->vp())
      result = result << 32;

    return result;
  }

  return 0;
}

void PluginAdapter::SetExpoAction(MultiActionList& expo)
{
  m_ExpoActionList = expo;
}

void PluginAdapter::SetScaleAction(MultiActionList& scale)
{
  m_ScaleActionList = scale;
}

std::string PluginAdapter::MatchStringForXids(std::vector<Window> const& windows)
{
  std::ostringstream sout;

  for (auto const& window : windows)
    sout << "xid=" << window << " | ";

  return sout.str();
}

void PluginAdapter::InitiateScale(std::string const& match, int state)
{
  if (!_spread_requested_state || !_scale_screen)
  {
    _spread_requested_state = true;
    CompOption::Vector argument(1);
    argument[0].setName("match", CompOption::TypeMatch);
    argument[0].value().set(CompMatch(match));
    m_ScaleActionList.InitiateAll(argument, state);
  }
  else
  {
    terminate_spread.emit();
    _scale_screen->relayoutSlots(CompMatch(match));
    initiate_spread.emit();
  }
}

void PluginAdapter::TerminateScale()
{
  m_ScaleActionList.TerminateAll();
  _spread_requested_state = false;
}

bool PluginAdapter::IsScaleActive() const
{
  return _spread_state;
}

bool PluginAdapter::IsScaleActiveForGroup() const
{
  return _spread_windows_state && _spread_state;
}

bool PluginAdapter::IsExpoActive() const
{
  return _expo_state;
}

bool PluginAdapter::IsWallActive() const
{
  return m_Screen->grabExist("wall");
}

bool PluginAdapter::IsAnyWindowMoving() const
{
    return m_Screen->grabExist("move");
}

void PluginAdapter::InitiateExpo()
{
  m_ExpoActionList.InitiateAll();
}

void PluginAdapter::TerminateExpo()
{
  m_ExpoActionList.Initiate("exit_button");
}

// WindowManager implementation
Window PluginAdapter::GetActiveWindow() const
{
  return m_Screen->activeWindow();
}

std::vector<Window> PluginAdapter::GetWindowsInStackingOrder() const
{
  bool stacking_order = true;
  auto const& windows = m_Screen->clientList(stacking_order);

  std::vector<Window> ret;
  for (auto const& window : windows)
    ret.push_back(window->id());

  return ret;
}

int PluginAdapter::MonitorGeometryIn(nux::Geometry const& geo) const
{
  std::vector<nux::Geometry> const& monitors = unity::UScreen::GetDefault()->GetMonitors();
  for (unsigned i = 0; i < monitors.size(); ++i)
  {
    nux::Geometry const& i_g = geo.Intersect(monitors[i]);

    if (i_g.width > geo.width/2 && i_g.height > geo.height/2)
      return i;
  }

  return 0;
}

bool PluginAdapter::IsTopWindowFullscreenOnMonitorWithMouse() const
{
  int monitor = unity::UScreen::GetDefault()->GetMonitorWithMouse();
  Window top_win = GetTopMostWindowInMonitor(monitor);
  CompWindow* window = m_Screen->findWindow(top_win);

  if (window)
    return (window->state() & CompWindowStateFullscreenMask);

  return false;
}

Window PluginAdapter::GetTopMostWindowInMonitor(int monitor) const
{
  CompWindow* window = nullptr;
  nux::Geometry const& m_geo = unity::UScreen::GetDefault()->GetMonitorGeometry(monitor);

  CompPoint screen_vp = m_Screen->vp();

  auto const& windows = m_Screen->windows();
  for (auto it = windows.rbegin(); it != windows.rend(); ++it)
  {
    window = *it;
    nux::Geometry const& win_geo = GetWindowGeometry(window->id());
    nux::Geometry const& intersect_geo = win_geo.Intersect(m_geo);

    if (intersect_geo.width > 0 &&
        intersect_geo.height > 0 &&
        window->defaultViewport() == screen_vp &&
        window->isViewable() && window->isMapped() &&
        !window->minimized() && !window->inShowDesktopMode() &&
        !(window->state() & CompWindowStateAboveMask) &&
        !(window->type() & CompWindowTypeSplashMask) &&
        !(window->type() & CompWindowTypeDockMask) &&
        !window->overrideRedirect())
    {
      return window->id();
    }
  }

  return 0;
}

bool PluginAdapter::IsWindowMaximized(Window window_id) const
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window)
  {
    return ((window->state() & MAXIMIZE_STATE) == MAXIMIZE_STATE);
  }

  return false;
}

bool PluginAdapter::IsWindowVerticallyMaximized(Window window_id) const
{
  if (CompWindow* window = m_Screen->findWindow(window_id))
    return (window->state() & CompWindowStateMaximizedVertMask);

  return false;
}

bool PluginAdapter::IsWindowHorizontallyMaximized(Window window_id) const
{
  if (CompWindow* window = m_Screen->findWindow(window_id))
    return (window->state() & CompWindowStateMaximizedHorzMask);

  return false;
}

bool PluginAdapter::IsWindowFullscreen(Window window_id) const
{
  if (CompWindow* window = m_Screen->findWindow(window_id))
    return ((window->state() & CompWindowStateFullscreenMask) == CompWindowStateFullscreenMask);

  return false;
}

bool PluginAdapter::HasWindowDecorations(Window window_id) const
{
  return compiz_utils::IsWindowFullyDecorable(m_Screen->findWindow(window_id));
}

bool PluginAdapter::IsWindowDecorated(Window window_id) const
{
  if (CompWindow* win = m_Screen->findWindow(window_id))
  {
    if ((win->state() & MAXIMIZE_STATE) != MAXIMIZE_STATE &&
        compiz_utils::IsWindowFullyDecorable(win))
    {
      return true;
    }
  }

  return false;
}

bool PluginAdapter::IsWindowOnCurrentDesktop(Window window_id) const
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window)
  {
    // we aren't checking window->onCurrentDesktop (), as the name implies, because that is broken
    return (window->defaultViewport() == m_Screen->vp());
  }

  return false;
}

bool PluginAdapter::IsWindowObscured(Window window_id) const
{
  if (_spread_state)
    return false;

  if (CompWindow* window = m_Screen->findWindow(window_id))
  {
    if (window->inShowDesktopMode())
      return true;

    CompPoint window_vp = window->defaultViewport();
    // Check if any windows above this one are blocking it
    for (CompWindow* sibling = window->next; sibling != NULL; sibling = sibling->next)
    {
      if (sibling->defaultViewport() == window_vp
          && !sibling->minimized()
          && sibling->isMapped()
          && sibling->isViewable()
          && (sibling->state() & MAXIMIZE_STATE) == MAXIMIZE_STATE
          && sibling->borderRect().intersects(window->borderRect()))
      {
        return true;
      }
    }
  }

  return false;
}

bool PluginAdapter::IsWindowMapped(Window window_id) const
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window)
    return window->mapNum () > 0;
  return true;
}

bool PluginAdapter::IsWindowVisible(Window window_id) const
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window)
    return !(window->state() & CompWindowStateHiddenMask) && !window->inShowDesktopMode();

  return false;
}

bool PluginAdapter::IsWindowShaded(Window window_id) const
{
  if (CompWindow* window = m_Screen->findWindow(window_id))
    return (window->state() & CompWindowStateShadedMask);

  return false;
}

bool PluginAdapter::IsWindowOnTop(Window window_id) const
{
  if (window_id == GetTopMostValidWindowInViewport())
    return true;

  return false;
}

Window PluginAdapter::GetTopMostValidWindowInViewport() const
{
  CompWindow* window;
  CompPoint screen_vp = m_Screen->vp();
  std::vector<Window> const& our_xids = nux::XInputWindow::NativeHandleList();

  auto const& windows = m_Screen->windows();
  for (auto it = windows.rbegin(); it != windows.rend(); ++it)
  {
    window = *it;
    if (window->defaultViewport() == screen_vp &&
        window->isViewable() && window->isMapped() &&
        !window->minimized() && !window->inShowDesktopMode() &&
        !((window->state() & CompWindowStateAboveMask) && !window->focused()) &&
        !(window->type() & CompWindowTypeSplashMask) &&
        !(window->type() & CompWindowTypeDockMask) &&
        !window->overrideRedirect() &&
        std::find(our_xids.begin(), our_xids.end(), window->id()) == our_xids.end())
    {
      return window->id();
    }
  }
  return 0;
}

bool PluginAdapter::IsCurrentViewportEmpty() const
{
  Window win = GetTopMostValidWindowInViewport();

  if (win)
  {
    CompWindow* cwin = m_Screen->findWindow(win);
    if (!(cwin->type() & NO_FOCUS_MASK))
      return false;
  }

  return true;
}

Window PluginAdapter::GetTopWindowAbove(Window xid) const
{
  CompWindow* window;
  CompPoint screen_vp = m_Screen->vp();

  auto const& windows = m_Screen->windows();
  for (auto it = windows.rbegin(); it != windows.rend(); ++it)
  {
    window = *it;
    if (window->defaultViewport() == screen_vp &&
        window->isViewable() && window->isMapped() &&
        !window->minimized() && !window->inShowDesktopMode() &&
        !(window->type() & CompWindowTypeDockMask) &&
        !(window->type() & CompWindowTypeSplashMask))
    {
      return window->id();
    }
    else if (window->id() == xid)
    {
      return 0;
    }
  }
  return 0;
}

bool PluginAdapter::IsWindowClosable(Window window_id) const
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window)
    return (window->actions() & CompWindowActionCloseMask);

  return false;
}

bool PluginAdapter::IsWindowMinimized(Window window_id) const
{
  if (CompWindow* window = m_Screen->findWindow(window_id))
    return window->minimized();

  return false;
}

bool PluginAdapter::IsWindowMinimizable(Window window_id) const
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window)
    return (window->actions() & CompWindowActionMinimizeMask);

  return false;
}

bool PluginAdapter::IsWindowMaximizable(Window window_id) const
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window)
    return (window->actions() & MAXIMIZABLE);

  return false;
}

void PluginAdapter::Maximize(Window window_id)
{
  if (CompWindow* window = m_Screen->findWindow(window_id))
    window->maximize(CompWindowStateMaximizedVertMask);
}

void PluginAdapter::VerticallyMaximize(Window window_id)
{
  if (CompWindow* window = m_Screen->findWindow(window_id))
    window->maximize(CompWindowStateMaximizedVertMask);
}

void PluginAdapter::HorizontallyMaximize(Window window_id)
{
  if (CompWindow* window = m_Screen->findWindow(window_id))
    window->maximize(CompWindowStateMaximizedHorzMask);
}

void PluginAdapter::Restore(Window window_id)
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window)
    window->maximize(0);
}

void PluginAdapter::RestoreAt(Window window_id, int x, int y)
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window && (window->state() & MAXIMIZE_STATE))
  {
    nux::Geometry new_geo(GetWindowSavedGeometry(window_id));
    window->maximize(0);

    auto const& border = window->border();
    new_geo.x = x;
    new_geo.y = y + border.top;
    new_geo.width -= (border.left + border.right);
    new_geo.height -= (border.top + border.bottom);

    MoveResizeWindow(window_id, new_geo);
  }
}

void PluginAdapter::ShowActionMenu(Time timestamp, Window window_id, unsigned button, nux::Point const& p)
{
  m_Screen->toolkitAction(Atoms::toolkitActionWindowMenu, timestamp, window_id, button, p.x, p.y);
}

void PluginAdapter::Minimize(Window window_id)
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window && (window->actions() & CompWindowActionMinimizeMask))
    window->minimize();
}

void PluginAdapter::UnMinimize(Window window_id)
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window && (window->actions() & CompWindowActionMinimizeMask))
  {
    window->unminimize();
    window->show();
  }
}

void PluginAdapter::Shade(Window window_id)
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window && (window->actions() & CompWindowActionShadeMask))
  {
    window->changeState(window->state() | CompWindowStateShadedMask);
    window->updateAttributes(CompStackingUpdateModeNone);
  }
}

void PluginAdapter::UnShade(Window window_id)
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window && (window->actions() & CompWindowActionShadeMask))
  {
    window->changeState(window->state() & ~CompWindowStateShadedMask);
    window->updateAttributes(CompStackingUpdateModeNone);
  }
}

void PluginAdapter::Close(Window window_id)
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window)
    window->close(CurrentTime);
}

void PluginAdapter::Activate(Window window_id)
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window)
    window->activate();
}

void PluginAdapter::Raise(Window window_id)
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window)
    window->raise();
}

void PluginAdapter::Lower(Window window_id)
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (window)
    window->lower();
}

void PluginAdapter::RestackBelow(Window window_id, Window sibiling_id)
{
  CompWindow* window = m_Screen->findWindow(window_id);
  if (!window)
    return;

  CompWindow* sibiling = m_Screen->findWindow(sibiling_id);
  if (sibiling)
    window->restackBelow(sibiling);
}

void PluginAdapter::FocusWindowGroup(std::vector<Window> const& window_ids,
                                     FocusVisibility focus_visibility,
                                     int monitor, bool only_top_win)
{
  CompPoint target_vp = m_Screen->vp();
  CompWindow* top_window = nullptr;
  CompWindow* top_monitor_win = nullptr;

  // mapped windows are visible windows (non-minimised)
  bool any_on_current = false;
  bool any_mapped = false;
  bool any_mapped_on_current = false;
  bool forced_unminimize = false;

  // Get the compiz windows for the window_ids passed in.
  CompWindowList windows;
  for (auto win : m_Screen->clientList())
  {
    Window id = win->id();
    if (std::find(window_ids.begin(), window_ids.end(), id) != window_ids.end())
      windows.push_back(win);
  }

  // Work out if there are any visible windows on the current view port.
  for (CompWindow* &win : windows)
  {
    if (win->defaultViewport() == target_vp)
    {
      any_on_current = true;
      if (!win->minimized())
        any_mapped_on_current = true;
    }

    if (!win->minimized())
      any_mapped = true;

    // If there is a visible window on the current view port, then don't
    // bother looking for others, as we will end up focusing the one on the
    // current view port, so we don't care if there are other windows on other
    // view ports.
    if (any_mapped_on_current)
      break;
  }

  // If we don't find any windows on the current view port, we need to look
  // for the view port of the maooed window.  Or if there are not any mapped
  // windows, then just choosing any view port is fine.
  if (!any_on_current)
  {
    // By iterating backwards, we stop at the top most window in the stack
    // that is either visible, or the top most minimised window.
    for (auto it = windows.rbegin(); it != windows.rend(); ++it)
    {
      CompWindow* win = *it;
      if ((any_mapped && !win->minimized()) || !any_mapped)
      {
        target_vp = win->defaultViewport();
        break;
      }
    }
  }

  for (CompWindow* &win : windows)
  {
    if (win->defaultViewport() == target_vp)
    {
      int win_monitor = GetWindowMonitor(win->id());

      /* Any window which is actually unmapped is
      * not going to be accessible by either switcher
      * or scale, so unconditionally unminimize those
      * windows when the launcher icon is activated */
      if ((focus_visibility == WindowManager::FocusVisibility::ForceUnminimizeOnCurrentDesktop &&
          target_vp == m_Screen->vp()) ||
          (focus_visibility == WindowManager::FocusVisibility::ForceUnminimizeInvisible &&
           win->mapNum() == 0))
      {
        top_window = win;
        forced_unminimize = true;

        if (monitor >= 0 && win_monitor == monitor)
          top_monitor_win = win;

        if (!only_top_win)
        {
          bool is_mapped = (win->mapNum() != 0);
          win->unminimize();

           /* Initially minimized windows dont get raised */
          if (!is_mapped)
            win->raise();
        }
      }
      else if ((any_mapped_on_current && !win->minimized()) || !any_mapped_on_current)
      {
        if (!forced_unminimize || target_vp == m_Screen->vp())
        {
          top_window = win;

          if (monitor >= 0 && win_monitor == monitor)
            top_monitor_win = win;

          if (!only_top_win)
            win->raise();
        }
      }
    }
  }

  if (monitor >= 0 && top_monitor_win)
    top_window = top_monitor_win;

  if (top_window)
  {
    if (only_top_win)
    {
      if (forced_unminimize)
        {
          top_window->unminimize();
          top_window->show();
        }

      top_window->raise();
    }

    top_window->activate();
  }
}

bool PluginAdapter::ScaleWindowGroup(std::vector<Window> const& windows, int state, bool force)
{
  std::size_t num_windows = windows.size();
  if (num_windows > 1 || (force && num_windows))
  {
    _spread_windows_state = true;
    std::string const& match = MatchStringForXids(windows);
    InitiateScale(match, state);
    return true;
  }
  return false;
}

void PluginAdapter::SetWindowIconGeometry(Window window, nux::Geometry const& geo)
{
  long data[4] = {geo.x, geo.y, geo.width, geo.height};

  XChangeProperty(m_Screen->dpy(), window, Atoms::wmIconGeometry,
                  XA_CARDINAL, 32, PropModeReplace,
                  (unsigned char*) data, 4);
}

void PluginAdapter::ShowDesktop()
{
  if (_in_show_desktop)
  {
    LOG_INFO(logger) << "Leaving show-desktop mode.";
    m_Screen->leaveShowDesktopMode(NULL);
  }
  else
  {
    LOG_INFO(logger) << "Entering show-desktop mode.";
    m_Screen->enterShowDesktopMode();
  }
}

bool PluginAdapter::InShowDesktop() const
{
  return _in_show_desktop;
}

void PluginAdapter::OnShowDesktop()
{
  _in_show_desktop = true;
  show_desktop_changed.emit();
}

void PluginAdapter::OnLeaveDesktop()
{
  _in_show_desktop = false;
  show_desktop_changed.emit();
}

void PluginAdapter::UpdateShowDesktopState()
{
  if (!IsCurrentViewportEmpty())
  {
    OnLeaveDesktop();
  }
  else
  {
    CompWindow* window;
    CompPoint screen_vp = m_Screen->vp();

    auto const& windows = m_Screen->windows();
    for (auto it = windows.rbegin(); it != windows.rend(); ++it)
    {
      window = *it;
      if (window->defaultViewport() == screen_vp &&
          window->inShowDesktopMode())
      {
        OnShowDesktop();
        break;
      }
    }
  }
}

int PluginAdapter::GetWindowMonitor(Window window_id) const
{
  // FIXME, we should use window->outputDevice() but this is not UScreen friendly
  nux::Geometry const& geo = GetWindowGeometry(window_id);

  if (!geo.IsNull())
  {
    int x = geo.x + geo.width/2;
    int y = geo.y + geo.height/2;

    return unity::UScreen::GetDefault()->GetMonitorAtPosition(x, y);
  }

  return -1;
}

nux::Geometry PluginAdapter::GetWindowGeometry(Window window_id) const
{
  if (CompWindow* window = m_Screen->findWindow(window_id))
  {
    auto const& b = window->borderRect();
    return nux::Geometry(b.x(), b.y(), b.width(), b.height());
  }

  return nux::Geometry();
}

nux::Geometry PluginAdapter::GetWindowSavedGeometry(Window window_id) const
{
  if (CompWindow* window = m_Screen->findWindow(window_id))
  {
    XWindowChanges &wc = window->saveWc();
    return nux::Geometry(wc.x, wc.y, wc.width, wc.height);
  }

  return nux::Geometry(0, 0, 1, 1);
}

nux::Geometry PluginAdapter::GetScreenGeometry() const
{
  return nux::Geometry(0, 0, m_Screen->width(), m_Screen->height());
}

template<typename T>
nux::Size get_edge_size(WindowManager::Edge edge, CompRect const& win_rect, T const& extents)
{
  switch (edge)
  {
    case WindowManager::Edge::LEFT:
      return nux::Size(extents.left, win_rect.height());
    case WindowManager::Edge::TOP:
      return nux::Size(win_rect.width(), extents.top);
    case WindowManager::Edge::RIGHT:
      return nux::Size(extents.right, win_rect.height());
    case WindowManager::Edge::BOTTOM:
      return nux::Size(win_rect.width(), extents.bottom);
  }

  return nux::Size();
}

nux::Size PluginAdapter::GetWindowDecorationSize(Window window_id, WindowManager::Edge edge) const
{
  if (CompWindow* window = m_Screen->findWindow(window_id))
  {
    if (compiz_utils::IsWindowFullyDecorable(window))
    {
      auto const& win_rect = window->borderRect();

      if ((window->state() & MAXIMIZE_STATE) == MAXIMIZE_STATE)
      {
        auto const& extents = decoration::Style::Get()->Border();
        nux::Geometry win_geo(win_rect.x(), win_rect.y(), win_rect.width(), win_rect.height());
        auto deco_size = get_edge_size(edge, win_rect, extents);
        double scale = Settings::Instance().em(MonitorGeometryIn(win_geo))->DPIScale();
        return nux::Size(deco_size.width * scale, deco_size.height * scale);
      }
      else
      {
        auto const& extents = window->border();
        return get_edge_size(edge, win_rect, extents);
      }
    }
  }

  return nux::Size();
}

nux::Geometry PluginAdapter::GetWorkAreaGeometry(Window window_id) const
{
  CompWindow* window = nullptr;
  unsigned int output = 0;

  if (window_id)
  {
    window = m_Screen->findWindow(window_id);
    if (window)
    {
      output = window->outputDevice();
    }
  }

  if (window_id == 0 || !window)
  {
    output = m_Screen->currentOutputDev().id();
  }

  CompRect workarea = m_Screen->getWorkareaForOutput(output);

  return nux::Geometry(workarea.x(), workarea.y(), workarea.width(), workarea.height());
}

bool PluginAdapter::CheckWindowIntersection(nux::Geometry const& region, CompWindow* window) const
{
  int intersect_types = CompWindowTypeNormalMask | CompWindowTypeDialogMask |
                        CompWindowTypeModalDialogMask | CompWindowTypeUtilMask;

  if (!window ||
      !(window->type() & intersect_types) ||
      !window->isMapped() ||
      !window->isViewable() ||
      window->state() & CompWindowStateHiddenMask)
    return false;

  if (CompRegion(window->borderRect()).intersects(CompRect(region.x, region.y, region.width, region.height)))
    return true;

  return false;
}

void PluginAdapter::CheckWindowIntersections(nux::Geometry const& region, bool &active, bool &any)
{
  // prime to false so we can assume values later one
  active = false;
  any = false;

  CompWindowList window_list = m_Screen->windows();
  CompWindowList::iterator it;
  CompWindow* window = NULL;
  CompWindow* parent = NULL;
  int type_dialogs = CompWindowTypeDialogMask | CompWindowTypeModalDialogMask
                     | CompWindowTypeUtilMask;


  window = m_Screen->findWindow(m_Screen->activeWindow());

  if (window && (window->type() & type_dialogs))
    parent = m_Screen->findWindow(window->transientFor());

  if (CheckWindowIntersection(region, window) || CheckWindowIntersection(region, parent))
  {
    any = true;
    active = true;
  }
  else
  {
    for (it = window_list.begin(); it != window_list.end(); ++it)
    {
      if (CheckWindowIntersection(region, *it))
      {
        any = true;
        break;
      }
    }
  }
}

int PluginAdapter::WorkspaceCount() const
{
  return m_Screen->vpSize().width() * m_Screen->vpSize().height();
}

void PluginAdapter::SetCurrentViewport(nux::Point const& vp)
{
  auto const& current = GetCurrentViewport();
  m_Screen->moveViewport(current.x - vp.x, current.y - vp.y, true);
}

nux::Point PluginAdapter::GetCurrentViewport() const
{
  CompPoint const& vp = m_Screen->vp();
  return nux::Point(vp.x(), vp.y());
}

void PluginAdapter::SetViewportSize(int horizontal, int vertical)
{
  if (horizontal < 1 || vertical < 1)
  {
    LOG_ERROR(logger) << "Impossible to set viewport to invalid values "
                      << horizontal << "x" << vertical;
    return;
  }

  CompOption::Value hsize(horizontal);
  m_Screen->setOptionForPlugin("core", "hsize", hsize);

  CompOption::Value vsize(vertical);
  m_Screen->setOptionForPlugin("core", "vsize", vsize);

  LOG_INFO(logger) << "Setting viewport size to " << hsize.i() << "x" << vsize.i();
}

int PluginAdapter::GetViewportHSize() const
{
  return m_Screen->vpSize().width();
}

int PluginAdapter::GetViewportVSize() const
{
  return m_Screen->vpSize().height();
}

bool PluginAdapter::IsScreenGrabbed() const
{
  if (m_Screen->grabbed())
    return true;

  auto* dpy = m_Screen->dpy();
  auto ret = XGrabKeyboard(dpy, m_Screen->root(), True, GrabModeAsync, GrabModeAsync, CurrentTime);

  if (ret == GrabSuccess)
  {
    XUngrabKeyboard(dpy, CurrentTime);
    XFlush(dpy);

    if (CompWindow* w = m_Screen->findWindow(m_Screen->activeWindow()))
      w->moveInputFocusTo();
  }

  return (ret == AlreadyGrabbed);
}

bool PluginAdapter::IsViewPortSwitchStarted() const
{
  return _vp_switch_started;
}

/* Returns true if the window was maximized */
bool PluginAdapter::MaximizeIfBigEnough(CompWindow* window) const
{
  XClassHint   classHint;
  Status       status;
  std::string  win_wmclass;
  int          num_monitor;

  int          screen_width;
  int          screen_height;
  float        covering_part;

  if (!window)
    return false;

  if ((window->state() & MAXIMIZE_STATE) == MAXIMIZE_STATE)
    return false;

  if (window->type() != CompWindowTypeNormalMask
      || (window->actions() & MAXIMIZABLE) != MAXIMIZABLE)
    return false;

  status = XGetClassHint(m_Screen->dpy(), window->id(), &classHint);
  if (status && classHint.res_class)
  {
    win_wmclass = classHint.res_class;
    XFree(classHint.res_class);

    if (classHint.res_name)
      XFree(classHint.res_name);
  }
  else
    return false;

  num_monitor = window->outputDevice();
  CompOutput &o = m_Screen->outputDevs().at(num_monitor);

  screen_height = o.workArea().height();
  screen_width = o.workArea().width();

  // See bug https://bugs.launchpad.net/unity/+bug/797808
  if (screen_height * screen_width > THRESHOLD_HEIGHT * THRESHOLD_WIDTH)
    return false;

  // use server<parameter> because the window won't show the real parameter as
  // not mapped yet
  const XSizeHints& hints = window->sizeHints();
  covering_part = (float)(window->serverWidth() * window->serverHeight()) / (float)(screen_width * screen_height);
  if ((covering_part < _coverage_area_before_automaximize) || (covering_part > 1.0) ||
      (hints.flags & PMaxSize && (screen_width > hints.max_width || screen_height > hints.max_height)))
  {
    LOG_DEBUG(logger) << win_wmclass << " window size doesn't fit";
    return false;
  }

  window->maximize(MAXIMIZE_STATE);

  return true;
}

void PluginAdapter::ShowGrabHandles(CompWindow* window, bool use_timer)
{
  if (!_grab_show_action || !window)
    return;

  CompOption::Vector argument(3);
  argument[0].setName("root", CompOption::TypeInt);
  argument[0].value().set((int) screen->root());
  argument[1].setName("window", CompOption::TypeInt);
  argument[1].value().set((int) window->id());
  argument[2].setName("use-timer", CompOption::TypeBool);
  argument[2].value().set(use_timer);

  /* Initiate the first available action with the arguments */
  _grab_show_action->initiate()(_grab_show_action, 0, argument);
}

void PluginAdapter::HideGrabHandles(CompWindow* window)
{
  if (!_grab_hide_action || !window)
    return;

  CompOption::Vector argument(2);
  argument[0].setName("root", CompOption::TypeInt);
  argument[0].value().set((int) screen->root());
  argument[1].setName("window", CompOption::TypeInt);
  argument[1].value().set((int) window->id());

  /* Initiate the first available action with the arguments */
  _grab_hide_action->initiate()(_grab_hide_action, 0, argument);
}

void PluginAdapter::ToggleGrabHandles(CompWindow* window)
{
  if (!_grab_toggle_action || !window)
    return;

  CompOption::Vector argument(2);
  argument[0].setName("root", CompOption::TypeInt);
  argument[0].value().set((int) screen->root());
  argument[1].setName("window", CompOption::TypeInt);
  argument[1].value().set((int) window->id());

  /* Initiate the first available action with the arguments */
  _grab_toggle_action->initiate()(_grab_toggle_action, 0, argument);
}

void PluginAdapter::SetCoverageAreaBeforeAutomaximize(float area)
{
  _coverage_area_before_automaximize = area;
}

bool PluginAdapter::SaveInputFocus()
{
  Window      active = m_Screen->activeWindow ();
  CompWindow* cw = m_Screen->findWindow (active);

  if (cw)
  {
    _last_focused_window = cw;
    return true;
  }

  return false;
}

bool PluginAdapter::RestoreInputFocus()
{
  if (_last_focused_window)
  {
    _last_focused_window->moveInputFocusTo ();
    _last_focused_window = NULL;
    return true;
  }
  else
  {
    m_Screen->focusDefaultWindow ();
    return false;
  }

  return false;
}

void PluginAdapter::MoveResizeWindow(Window window_id, nux::Geometry geometry)
{
  int w, h;
  CompWindow* window = m_Screen->findWindow(window_id);

  if (!window)
    return;

  if (window->constrainNewWindowSize(geometry.width, geometry.height, &w, &h))
  {
    CompRect workarea = m_Screen->getWorkareaForOutput(window->outputDevice());
    int dx = geometry.x + w - workarea.right() + window->border().right;
    int dy = geometry.y + h - workarea.bottom() + window->border().bottom;

    if (dx > 0)
      geometry.x -= dx;
    if (dy > 0)
      geometry.y -= dy;

    geometry.SetWidth(w);
    geometry.SetHeight(h);
  }

  XWindowChanges xwc;
  xwc.x = geometry.x;
  xwc.y = geometry.y;
  xwc.width = geometry.width;
  xwc.height = geometry.height;

  if (window->mapNum())
    window->sendSyncRequest();

  window->configureXWindow(CWX | CWY | CWWidth | CWHeight, &xwc);
}

void PluginAdapter::OnWindowClosed(CompWindow *w)
{
  if (_last_focused_window == w)
    _last_focused_window = NULL;
}

Cursor PluginAdapter::GetCachedCursor(unsigned int cursor_name) const
{
  return screen->cursorCache(cursor_name);
}

bool PluginAdapter::IsNuxWindow(CompWindow* value)
{
  std::vector<Window> const& xwns = nux::XInputWindow::NativeHandleList();
  auto id = value->id();

  unsigned int size = xwns.size();
  for (unsigned int i = 0; i < size; ++i)
  {
    if (xwns[i] == id)
      return true;
  }
  return false;
}

void PluginAdapter::AddProperties(debug::IntrospectionData& wrapper)
{
  wrapper.add(GetScreenGeometry())
         .add("workspace_count", WorkspaceCount())
         .add("active_window", GetActiveWindow())
         .add("screen_grabbed", IsScreenGrabbed())
         .add("scale_active", IsScaleActive())
         .add("scale_active_for_group", IsScaleActiveForGroup())
         .add("expo_active", IsExpoActive())
         .add("viewport_switch_running", IsViewPortSwitchStarted())
         .add("showdesktop_active", _in_show_desktop);
}

} // namespace unity
