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
#include <sstream>
#include "PluginAdapter.h"
#include "UScreen.h"

#include <NuxCore/Logger.h>
#include <UnityCore/Variant.h>

namespace unity
{
DECLARE_LOGGER(logger, "unity.wm.compiz");
namespace
{
const int THRESHOLD_HEIGHT = 600;
const int THRESHOLD_WIDTH = 1024;
const char* _UNITY_FRAME_EXTENTS = "_UNITY_FRAME_EXTENTS";

std::shared_ptr<PluginAdapter> global_instance;
}

#define MAXIMIZABLE (CompWindowActionMaximizeHorzMask & CompWindowActionMaximizeVertMask & CompWindowActionResizeMask)

#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_UNDECORATED_UNITY 0x80


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
void PluginAdapter::Initialize(CompScreen* screen)
{
  global_instance.reset(new PluginAdapter(screen));
}

PluginAdapter::PluginAdapter(CompScreen* screen) :
  m_Screen(screen),
  _in_show_desktop (false),
  _last_focused_window(nullptr)
{
  _spread_state = false;
  _spread_windows_state = false;
  _expo_state = false;
  _vp_switch_started = false;

  _grab_show_action = 0;
  _grab_hide_action = 0;
  _grab_toggle_action = 0;
  _coverage_area_before_automaximize = 0.75;
  bias_active_to_viewport = false;
}

PluginAdapter::~PluginAdapter()
{
}

/* A No-op for now, but could be useful later */
void PluginAdapter::OnScreenGrabbed()
{
  screen_grabbed.emit();

  if (!_spread_state && screen->grabExist("scale"))
  {
    _spread_state = true;
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
}

void PluginAdapter::NotifyNewDecorationState(Window xid)
{
  auto deco_state_it = _window_decoration_state.find(xid);
  bool wasTracked = (deco_state_it != _window_decoration_state.end());
  bool wasDecorated = false;

  if (wasTracked)
  {
    wasDecorated = deco_state_it->second;
    _window_decoration_state.erase(deco_state_it);
  }

  bool decorated = HasWindowDecorations(xid);

  if (decorated == wasDecorated)
    return;

  if (decorated && (!wasDecorated || !wasTracked))
  {
    window_decorated.emit(xid);
  }
  else if (wasDecorated || !wasTracked)
  {
    window_undecorated.emit(xid);
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
  else if (IsScaleActive() && g_strcmp0(plugin, "scale") == 0 &&
           g_strcmp0(event, "activate") == 0)
  {
    // If the scale plugin is activated again while is already grabbing the screen
    // it means that is switching the view (i.e. switching from a spread application
    // to another), so we need to notify our clients that it has really terminated
    // and initiated again.
    terminate_spread.emit();
    initiate_spread.emit();
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
  action->initiate()(action, state, argument);
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
    primary_action_->terminate()(primary_action_, 0, argument);
    return;
  }

  for (auto const& it : actions_)
  {
    CompAction* action = it.second;

    if (action->state() & (CompAction::StateTermKey |
                           CompAction::StateTermButton |
                           CompAction::StateTermEdge |
                           CompAction::StateTermEdgeDnd))
    {
      action->terminate()(action, 0, argument);
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

  sout << "any & (";

  for (auto const& window : windows)
  {
    sout << "| xid=" << window << " ";
  }
  sout << ")";

  return sout.str();
}

void PluginAdapter::InitiateScale(std::string const& match, int state)
{
  CompOption::Vector argument(1);
  argument[0].setName("match", CompOption::TypeMatch);
  argument[0].value().set(CompMatch(match));

  m_ScaleActionList.InitiateAll(argument, state);
}

void PluginAdapter::TerminateScale()
{
  m_ScaleActionList.TerminateAll();
}

bool PluginAdapter::IsScaleActive() const
{
  return m_Screen->grabExist("scale");
}

bool PluginAdapter::IsScaleActiveForGroup() const
{
  return _spread_windows_state && m_Screen->grabExist("scale");
}

bool PluginAdapter::IsExpoActive() const
{
  return m_Screen->grabExist("expo");
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

unsigned long PluginAdapter::GetMwnDecorations(Window window_id) const
{
  Display* display = m_Screen->dpy();
  MotifWmHints* hints = NULL;
  Atom type = None;
  gint format;
  gulong nitems;
  gulong bytes_after;
  unsigned long decorations = 0;

  if (XGetWindowProperty(display, window_id, Atoms::mwmHints, 0,
                         sizeof(MotifWmHints) / sizeof(long), False,
                         Atoms::mwmHints, &type, &format, &nitems, &bytes_after,
                         (guchar**)&hints) != Success)
  {
    return decorations;
  }

  decorations |= (MwmDecorAll | MwmDecorTitle);

  if (!hints)
    return decorations;

  /* Check for the presence of the high bit
   * if present, it means that we undecorated
   * this window, so don't mark it as undecorated */
  if (type == Atoms::mwmHints && format != 0 && hints->flags & MWM_HINTS_DECORATIONS)
  {
    decorations = hints->decorations;
  }

  XFree(hints);
  return decorations;
}

bool PluginAdapter::HasWindowDecorations(Window window_id) const
{
  auto deco_state_it = _window_decoration_state.find(window_id);

  if (deco_state_it != _window_decoration_state.end())
    return deco_state_it->second;

  unsigned long decorations = GetMwnDecorations(window_id);

  /* Must have both bits set */
  bool decorated = (decorations & (MwmDecorAll | MwmDecorTitle)) ||
                   (decorations & MWM_HINTS_UNDECORATED_UNITY);

  _window_decoration_state[window_id] = decorated;

  return decorated;
}

bool PluginAdapter::IsWindowDecorated(Window window_id) const
{
  bool decorated = GetMwnDecorations(window_id) & (MwmDecorAll | MwmDecorTitle);

  if (decorated && GetCardinalProperty(window_id, Atoms::frameExtents).empty())
  {
    decorated = false;
  }

  return decorated;
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
  CompWindow* window = m_Screen->findWindow(window_id);

  if (window)
  {
    if (window->inShowDesktopMode())
      return true;

    CompPoint window_vp = window->defaultViewport();
    nux::Geometry const& win_geo = GetWindowGeometry(window->id());
    // Check if any windows above this one are blocking it
    for (CompWindow* sibling = window->next; sibling != NULL; sibling = sibling->next)
    {
      if (sibling->defaultViewport() == window_vp
          && !sibling->minimized()
          && sibling->isMapped()
          && sibling->isViewable()
          && (sibling->state() & MAXIMIZE_STATE) == MAXIMIZE_STATE
          && !GetWindowGeometry(sibling->id()).Intersect(win_geo).IsNull())
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
        !(window->state() & CompWindowStateAboveMask) &&
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
    window->maximize(MAXIMIZE_STATE);
}

void PluginAdapter::VerticallyMaximizeWindowAt(CompWindow* window, nux::Geometry const& geo)
{
  if (window && ((window->type() & CompWindowTypeNormalMask) ||
      ((window->actions() & CompWindowActionMaximizeVertMask) &&
        window->actions() & CompWindowActionResizeMask)))
  {
    /* First we unmaximize the Window */
    if (window->state() & MAXIMIZE_STATE)
      window->maximize(0);

    /* Then we vertically maximize the it so it can be unminimized correctly */
    if (!(window->state() & CompWindowStateMaximizedVertMask))
      window->maximize(CompWindowStateMaximizedVertMask);

    /* Then we resize and move it on the requested place */
    MoveResizeWindow(window->id(), geo);
  }
}

void PluginAdapter::LeftMaximize(Window window_id)
{
  CompWindow* window = m_Screen->findWindow(window_id);

  if (!window)
    return;

  /* Let's compute the area where the window should stay */
  CompRect workarea = m_Screen->getWorkareaForOutput(window->outputDevice());
  nux::Geometry win_geo(workarea.x() + window->border().left,
                        workarea.y() + window->border().top,
                        workarea.width() / 2 - (window->border().left + window->border().right),
                        workarea.height() - (window->border().top + window->border().bottom));

  VerticallyMaximizeWindowAt(window, win_geo);
}

void PluginAdapter::RightMaximize(Window window_id)
{
  CompWindow* window = m_Screen->findWindow(window_id);

  if (!window)
    return;

  /* Let's compute the area where the window should stay */
  CompRect workarea = m_Screen->getWorkareaForOutput(window->outputDevice());
  nux::Geometry win_geo(workarea.x() + workarea.width() / 2 + window->border().left,
                        workarea.y() + window->border().top,
                        workarea.width() / 2 - (window->border().left + window->border().right),
                        workarea.height() - (window->border().top + window->border().bottom));

  VerticallyMaximizeWindowAt(window, win_geo);
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
    new_geo.x = x;
    new_geo.y = y;
    window->maximize(0);
    MoveResizeWindow(window_id, new_geo);
  }
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
    window->unminimize();
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
    std::string const& match = MatchStringForXids(windows);
    InitiateScale(match, state);
    _spread_windows_state = true;
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
}

void PluginAdapter::OnLeaveDesktop()
{
  _in_show_desktop = false;
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

nux::Size PluginAdapter::GetWindowDecorationSize(Window window_id, WindowManager::Edge edge) const
{
  if (CompWindow* window = m_Screen->findWindow(window_id))
  {
    if (HasWindowDecorations(window_id))
    {
      auto const& win_rect = window->borderRect();

      if (IsWindowDecorated(window_id))
      {
        auto const& extents = window->border();

        switch (edge)
        {
          case Edge::LEFT:
            return nux::Size(extents.left, win_rect.height());
          case Edge::TOP:
            return nux::Size(win_rect.width(), extents.top);
          case Edge::RIGHT:
            return nux::Size(extents.right, win_rect.height());
          case Edge::BOTTOM:
            return nux::Size(win_rect.width(), extents.bottom);
        }
      }
      else
      {
        Atom unity_extents = gdk_x11_get_xatom_by_name(_UNITY_FRAME_EXTENTS);
        auto const& extents = GetCardinalProperty(window_id, unity_extents);

        if (extents.size() == 4)
        {
          switch (edge)
          {
            case Edge::LEFT:
              return nux::Size(extents[unsigned(Edge::LEFT)], win_rect.height());
            case Edge::TOP:
              return nux::Size(win_rect.width(), extents[unsigned(Edge::TOP)]);
            case Edge::RIGHT:
              return nux::Size(extents[unsigned(Edge::RIGHT)], win_rect.height());
            case Edge::BOTTOM:
              return nux::Size(win_rect.width(), extents[unsigned(Edge::BOTTOM)]);
          }
        }
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

  CompOption::Value hsize;
  hsize.set<int>(horizontal);
  m_Screen->setOptionForPlugin("core", "hsize", hsize);

  CompOption::Value vsize(vertical);
  vsize.set<int>(vertical);
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

void PluginAdapter::SetMwmWindowHints(Window xid, MotifWmHints* new_hints) const
{
  Display* display = m_Screen->dpy();
  MotifWmHints* data = NULL;
  MotifWmHints* hints = NULL;
  Atom type = None;
  gint format;
  gulong nitems;
  gulong bytes_after;

  if (XGetWindowProperty(display,
                         xid,
                         Atoms::mwmHints, 0, sizeof(MotifWmHints) / sizeof(long),
                         False, AnyPropertyType, &type, &format, &nitems,
                         &bytes_after, (guchar**)&data) != Success)
  {
    return;
  }

  if (type != Atoms::mwmHints || !data)
  {
    hints = new_hints;
  }
  else
  {
    hints = data;

    if (new_hints->flags & MWM_HINTS_FUNCTIONS)
    {
      hints->flags |= MWM_HINTS_FUNCTIONS;
      hints->functions = new_hints->functions;
    }
    if (new_hints->flags & MWM_HINTS_DECORATIONS)
    {
      hints->flags |= MWM_HINTS_DECORATIONS;
      hints->decorations = new_hints->decorations;
    }
  }

  XChangeProperty(display, xid, Atoms::mwmHints, Atoms::mwmHints, 32, PropModeReplace,
                  (guchar*)hints, sizeof(MotifWmHints) / sizeof(long));

  if (data)
    XFree(data);
}

void PluginAdapter::Decorate(Window window_id) const
{
  if (!HasWindowDecorations(window_id))
    return;

  MotifWmHints hints = { 0 };

  hints.flags = MWM_HINTS_DECORATIONS;
  hints.decorations = GDK_DECOR_ALL & ~(MWM_HINTS_UNDECORATED_UNITY);

  SetMwmWindowHints(window_id, &hints);

  // Removing the saved windows extents
  Atom atom = gdk_x11_get_xatom_by_name(_UNITY_FRAME_EXTENTS);
  XDeleteProperty(m_Screen->dpy(), window_id, atom);
}

void PluginAdapter::Undecorate(Window window_id) const
{
  if (!IsWindowDecorated(window_id))
    return;

  if (CompWindow* window = m_Screen->findWindow(window_id))
  {
    // Saving the previous window extents values
    long extents[4];
    auto const& border = window->border();
    extents[unsigned(Edge::LEFT)] = border.left;
    extents[unsigned(Edge::RIGHT)] = border.right;
    extents[unsigned(Edge::TOP)] = border.top;
    extents[unsigned(Edge::BOTTOM)] = border.bottom;

    Atom atom = gdk_x11_get_xatom_by_name(_UNITY_FRAME_EXTENTS);
    XChangeProperty(m_Screen->dpy(), window_id, atom, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char*) extents, 4);
  }

  MotifWmHints hints = { 0 };

  /* Set the high bit to indicate that we undecorated this
   * window, when an application attempts to "undecorate"
   * the window again, this bit will be cleared */
  hints.flags = MWM_HINTS_DECORATIONS;
  hints.decorations = MWM_HINTS_UNDECORATED_UNITY;

  SetMwmWindowHints(window_id, &hints);
}

bool PluginAdapter::IsScreenGrabbed() const
{
  return m_Screen->grabbed();
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

void PluginAdapter::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper wrapper(builder);
  wrapper.add(GetScreenGeometry())
         .add("workspace_count", WorkspaceCount())
         .add("active_window", (uint64_t)GetActiveWindow())
         .add("screen_grabbed", IsScreenGrabbed())
         .add("scale_active", IsScaleActive())
         .add("scale_active_for_group", IsScaleActiveForGroup())
         .add("expo_active", IsExpoActive())
         .add("viewport_switch_running", IsViewPortSwitchStarted())
         .add("showdesktop_active", _in_show_desktop);
}

std::string PluginAdapter::GetWindowName(Window window_id) const
{
  std::string name;
  Atom visibleNameAtom;

  visibleNameAtom = gdk_x11_get_xatom_by_name("_NET_WM_VISIBLE_NAME");
  name = GetUtf8Property(window_id, visibleNameAtom);

  if (name.empty())
    name = GetUtf8Property(window_id, Atoms::wmName);

  if (name.empty())
    name = GetTextProperty(window_id, XA_WM_NAME);

  return name;
}

std::string PluginAdapter::GetUtf8Property(Window window_id, Atom atom) const
{
  Atom type;
  int result, format;
  unsigned long n_items, bytes_after;
  char *val = nullptr;
  std::string retval;

  result = XGetWindowProperty(m_Screen->dpy(), window_id, atom, 0L, 65536, False,
                              Atoms::utf8String, &type, &format, &n_items,
                              &bytes_after, reinterpret_cast<unsigned char **>(&val));

  if (result != Success)
    return retval;

  if (type == Atoms::utf8String && format == 8 && val && n_items > 0)
  {
    retval = std::string(val, n_items);
  }

  XFree(val);

  return retval;
}

std::string PluginAdapter::GetTextProperty(Window window_id, Atom atom) const
{
  XTextProperty text;
  std::string retval;
  text.nitems = 0;

  if (XGetTextProperty(m_Screen->dpy(), window_id, &text, atom))
  {
    if (text.value)
    {
      retval = std::string(reinterpret_cast<char*>(text.value), text.nitems);
      XFree(text.value);
    }
  }

  return retval;
}

std::vector<long> PluginAdapter::GetCardinalProperty(Window window_id, Atom atom) const
{
  Atom type;
  int result, format;
  unsigned long n_items, bytes_after;
  long *buf = nullptr;

  result = XGetWindowProperty(m_Screen->dpy(), window_id, atom, 0L, 65536, False,
                              XA_CARDINAL, &type, &format, &n_items, &bytes_after,
                              reinterpret_cast<unsigned char **>(&buf));

  std::unique_ptr<long[], int(*)(void*)> buffer(buf, XFree);

  if (result == Success && type == XA_CARDINAL && format == 32 && buffer && n_items > 0)
  {
    std::vector<long> values(n_items);

    for (unsigned i = 0; i < n_items; ++i)
      values[i] = buffer[i];

    return values;
  }

  return std::vector<long>();
}

} // namespace unity
