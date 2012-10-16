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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#include <glib.h>
#include <sstream>
#include "PluginAdapter.h"
#include "UScreen.h"

#include <NuxCore/Logger.h>
#include <UnityCore/Variant.h>

namespace
{

nux::logging::Logger logger("unity.plugin");

const int THRESHOLD_HEIGHT = 600;
const int THRESHOLD_WIDTH = 1024;

}

PluginAdapter* PluginAdapter::_default = 0;

#define MAXIMIZABLE (CompWindowActionMaximizeHorzMask & CompWindowActionMaximizeVertMask & CompWindowActionResizeMask)

#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_UNDECORATED_UNITY 0x80
#define _XA_MOTIF_WM_HINTS    "_MOTIF_WM_HINTS"

/* static */
PluginAdapter*
PluginAdapter::Default()
{
  if (!_default)
    return 0;
  return _default;
}

/* static */
void
PluginAdapter::Initialize(CompScreen* screen)
{
  _default = new PluginAdapter(screen);
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
void
PluginAdapter::OnScreenGrabbed()
{
  compiz_screen_grabbed.emit();

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

void
PluginAdapter::OnScreenUngrabbed()
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

  compiz_screen_ungrabbed.emit();
}

void
PluginAdapter::NotifyResized(CompWindow* window, int x, int y, int w, int h)
{
  window_resized.emit(window->id());
}

void
PluginAdapter::NotifyMoved(CompWindow* window, int x, int y)
{
  window_moved.emit(window->id());
}

void
PluginAdapter::NotifyStateChange(CompWindow* window, unsigned int state, unsigned int last_state)
{
  if (!((last_state & MAXIMIZE_STATE) == MAXIMIZE_STATE)
      && ((state & MAXIMIZE_STATE) == MAXIMIZE_STATE))
  {
    WindowManager::window_maximized.emit(window->id());
  }
  else if (((last_state & MAXIMIZE_STATE) == MAXIMIZE_STATE)
           && !((state & MAXIMIZE_STATE) == MAXIMIZE_STATE))
  {
    WindowManager::window_restored.emit(window->id());
  }
}

void
PluginAdapter::NotifyNewDecorationState(guint32 xid)
{
  bool wasTracked = (_window_decoration_state.find (xid) != _window_decoration_state.end ());
  bool wasDecorated = false;

  if (wasTracked)
    wasDecorated = _window_decoration_state[xid];

  bool decorated = IsWindowDecorated (xid);

  if (decorated == wasDecorated)
    return;

  if (decorated && (!wasDecorated || !wasTracked))
    WindowManager::window_decorated.emit(xid);
  else if (wasDecorated || !wasTracked)
    WindowManager::window_undecorated.emit(xid);
}

void
PluginAdapter::Notify(CompWindow* window, CompWindowNotify notify)
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
      WindowManager::window_mapped.emit(window->id());
      break;
    case CompWindowNotifyUnmap:
      WindowManager::window_unmapped.emit(window->id());
      break;
    case CompWindowNotifyFocusChange:
      WindowManager::window_focus_changed.emit(window->id());
      break;
    default:
      break;
  }
}

void
PluginAdapter::NotifyCompizEvent(const char* plugin, const char* event, CompOption::Vector& option)
{
  if (g_strcmp0(event, "start_viewport_switch") == 0)
  {
    _vp_switch_started = true;
    compiz_screen_viewport_switch_started.emit();
  }
  else if (g_strcmp0(event, "end_viewport_switch") == 0)
  {
    _vp_switch_started = false;
    compiz_screen_viewport_switch_ended.emit();
  }

  compiz_event.emit(plugin, event, option);
}

void
MultiActionList::AddNewAction(std::string const& name, CompAction* a, bool primary)
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

unsigned long long
PluginAdapter::GetWindowActiveNumber (guint32 xid) const
{
  Window win = xid;
  CompWindow* window;

  window = m_Screen->findWindow(win);

  if (window)
  {
    // result is actually an unsigned int (32 bits)
    unsigned long long result = window->activeNum ();
    if (bias_active_to_viewport() && window->defaultViewport() == m_Screen->vp())
      result = result << 32;

    return result;
  }

  return 0;
}

void
PluginAdapter::SetExpoAction(MultiActionList& expo)
{
  m_ExpoActionList = expo;
}

void
PluginAdapter::SetScaleAction(MultiActionList& scale)
{
  m_ScaleActionList = scale;
}

std::string
PluginAdapter::MatchStringForXids(std::vector<Window> const& windows)
{
  std::string out_string = "any & (";

  for (auto const& xid : windows)
    out_string += "| xid=" + std::to_string(xid) + " ";

  out_string += ")";

  return out_string;
}

void
PluginAdapter::InitiateScale(std::string const& match, int state)
{
  CompOption::Vector argument(1);
  argument[0].setName("match", CompOption::TypeMatch);
  argument[0].value().set(CompMatch(match));

  m_ScaleActionList.InitiateAll(argument, state);
}

void
PluginAdapter::TerminateScale()
{
  m_ScaleActionList.TerminateAll();
}

bool
PluginAdapter::IsScaleActive() const
{
  return m_Screen->grabExist("scale");
}

bool
PluginAdapter::IsScaleActiveForGroup() const
{
  return _spread_windows_state && m_Screen->grabExist("scale");
}

bool
PluginAdapter::IsExpoActive() const
{
  return m_Screen->grabExist("expo");
}

bool
PluginAdapter::IsWallActive() const
{
  return m_Screen->grabExist("wall");
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
guint32
PluginAdapter::GetActiveWindow() const
{
  return m_Screen->activeWindow();
}

bool
PluginAdapter::IsWindowMaximized(guint xid) const
{
  Window win = xid;
  CompWindow* window;

  window = m_Screen->findWindow(win);
  if (window)
  {
    return ((window->state() & MAXIMIZE_STATE) == MAXIMIZE_STATE);
  }

  return false;
}

bool
PluginAdapter::IsWindowDecorated(guint32 xid)
{
  Display* display = m_Screen->dpy();
  Window win = xid;
  Atom hints_atom = None;
  MotifWmHints* hints = NULL;
  Atom type = None;
  gint format;
  gulong nitems;
  gulong bytes_after;
  bool ret = true;

  hints_atom = XInternAtom(display, _XA_MOTIF_WM_HINTS, false);

  if (XGetWindowProperty(display, win, hints_atom, 0,
                         sizeof(MotifWmHints) / sizeof(long), False,
                         hints_atom, &type, &format, &nitems, &bytes_after,
                         (guchar**)&hints) != Success)
    return false;

  if (!hints)
    return ret;

  /* Check for the presence of the high bit
   * if present, it means that we undecorated
   * this window, so don't mark it as undecorated */
  if (type == hints_atom && format != 0 &&
      hints->flags & MWM_HINTS_DECORATIONS)
  {
    /* Must have both bits set */
    _window_decoration_state[xid] = ret =
          (hints->decorations & (MwmDecorAll | MwmDecorTitle))  ||
          (hints->decorations & MWM_HINTS_UNDECORATED_UNITY);
  }

  XFree(hints);
  return ret;
}

bool
PluginAdapter::IsWindowOnCurrentDesktop(guint32 xid) const
{
  Window win = xid;
  CompWindow* window;

  window = m_Screen->findWindow(win);
  if (window)
  {
    // we aren't checking window->onCurrentDesktop (), as the name implies, because that is broken
    return (window->defaultViewport() == m_Screen->vp());
  }

  return false;
}

bool
PluginAdapter::IsWindowObscured(guint32 xid) const
{
  Window win = xid;
  CompWindow* window;

  window = m_Screen->findWindow(win);

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

bool
PluginAdapter::IsWindowMapped(guint32 xid) const
{
  Window win = xid;
  CompWindow* window;

  window = m_Screen->findWindow(win);
  if (window)
    return window->mapNum () > 0;
  return true;
}

bool
PluginAdapter::IsWindowVisible(guint32 xid) const
{
  Window win = xid;
  CompWindow* window;

  window = m_Screen->findWindow(win);
  if (window)
    return !(window->state() & CompWindowStateHiddenMask) && !window->inShowDesktopMode();

  return false;
}

bool
PluginAdapter::IsWindowOnTop(guint32 xid) const
{
  if (xid == GetTopMostValidWindowInViewport())
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
        !(window->type() & CompWindowTypeSplashMask) &&
        !(window->type() & CompWindowTypeDockMask))
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

bool
PluginAdapter::IsWindowClosable(guint32 xid) const
{
  Window win = xid;
  CompWindow* window;

  window = m_Screen->findWindow(win);
  if (window)
    return (window->actions() & CompWindowActionCloseMask);

  return false;
}

bool
PluginAdapter::IsWindowMinimizable(guint32 xid) const
{
  Window win = xid;
  CompWindow* window;

  window = m_Screen->findWindow(win);
  if (window)
    return (window->actions() & CompWindowActionMinimizeMask);

  return false;
}

bool
PluginAdapter::IsWindowMaximizable(guint32 xid) const
{
  Window win = xid;
  CompWindow* window;

  window = m_Screen->findWindow(win);
  if (window)
    return (window->actions() & MAXIMIZABLE);

  return false;
}

void
PluginAdapter::Restore(guint32 xid)
{
  Window win = xid;
  CompWindow* window;

  window = m_Screen->findWindow(win);
  if (window)
    window->maximize(0);
}

void
PluginAdapter::RestoreAt(guint32 xid, int x, int y)
{
  Window win = xid;
  CompWindow* window;

  window = m_Screen->findWindow(win);
  if (window && (window->state() & MAXIMIZE_STATE))
  {
    nux::Geometry new_geo(GetWindowSavedGeometry(xid));
    new_geo.x = x;
    new_geo.y = y;
    window->maximize(0);
    MoveResizeWindow(xid, new_geo);
  }
}

void
PluginAdapter::Minimize(guint32 xid)
{
  Window win = xid;
  CompWindow* window;

  window = m_Screen->findWindow(win);
  if (window && (window->actions() & CompWindowActionMinimizeMask))
    window->minimize();
}

void
PluginAdapter::Close(guint32 xid)
{
  Window win = xid;
  CompWindow* window;

  window = m_Screen->findWindow(win);
  if (window)
    window->close(CurrentTime);
}

void
PluginAdapter::Activate(guint32 xid)
{
  Window win = xid;
  CompWindow* window;

  window = m_Screen->findWindow(win);
  if (window)
    window->activate();
}

void
PluginAdapter::Raise(guint32 xid)
{
  Window win = xid;
  CompWindow* window;

  window = m_Screen->findWindow(win);
  if (window)
    window->raise();
}

void
PluginAdapter::Lower(guint32 xid)
{
  Window win = xid;
  CompWindow* window;

  window = m_Screen->findWindow(win);
  if (window)
    window->lower();
}

void
PluginAdapter::FocusWindowGroup(std::vector<Window> window_ids, FocusVisibility focus_visibility, int monitor, bool only_top_win)
{
  CompPoint target_vp = m_Screen->vp();
  CompWindow* top_window = nullptr;
  CompWindow* top_monitor_win = nullptr;

  bool any_on_current = false;
  bool any_mapped = false;
  bool any_mapped_on_current = false;
  bool forced_unminimize = false;

  /* sort the list */
  CompWindowList windows;
  for (auto win : m_Screen->clientList())
  {
    Window id = win->id();
    if (std::find(window_ids.begin(), window_ids.end(), id) != window_ids.end())
      windows.push_back(win);
  }

  /* filter based on workspace */
  for (CompWindow* &win : windows)
  {
    if (win->defaultViewport() == m_Screen->vp())
    {
      any_on_current = true;

      if (!win->minimized())
      {
        any_mapped_on_current = true;
      }
    }

    if (!win->minimized())
    {
      any_mapped = true;
    }

    if (any_on_current && any_mapped)
      break;
  }

  if (!any_on_current)
  {
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

bool
PluginAdapter::ScaleWindowGroup(std::vector<Window> windows, int state, bool force)
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

void
PluginAdapter::SetWindowIconGeometry(Window window, nux::Geometry const& geo)
{
  long data[4];

  data[0] = geo.x;
  data[1] = geo.y;
  data[2] = geo.width;
  data[3] = geo.height;

  XChangeProperty(m_Screen->dpy(), window, Atoms::wmIconGeometry,
                  XA_CARDINAL, 32, PropModeReplace,
                  (unsigned char*) data, 4);
}

void
PluginAdapter::ShowDesktop()
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

void
PluginAdapter::OnShowDesktop()
{
  LOG_DEBUG(logger) << "Now in show desktop mode.";
  _in_show_desktop = true;
}

void
PluginAdapter::OnLeaveDesktop()
{
  LOG_DEBUG(logger) << "No longer in show desktop mode.";
  _in_show_desktop = false;
}

int
PluginAdapter::GetWindowMonitor(guint32 xid) const
{
  // FIXME, we should use window->outputDevice() but this is not UScreen friendly
  nux::Geometry const& geo = GetWindowGeometry(xid);

  if (!geo.IsNull())
  {
    int x = geo.x + geo.width/2;
    int y = geo.y + geo.height/2;

    return unity::UScreen::GetDefault()->GetMonitorAtPosition(x, y);
  }

  return -1;
}

nux::Geometry
PluginAdapter::GetWindowGeometry(guint32 xid) const
{
  Window win = xid;
  CompWindow* window;
  nux::Geometry geo;

  window = m_Screen->findWindow(win);
  if (window)
  {
    geo.x = window->borderRect().x();
    geo.y = window->borderRect().y();
    geo.width = window->borderRect().width();
    geo.height = window->borderRect().height();
  }
  return geo;
}

nux::Geometry
PluginAdapter::GetWindowSavedGeometry(guint32 xid) const
{
  Window win = xid;
  nux::Geometry geo(0, 0, 1, 1);
  CompWindow* window;

  window = m_Screen->findWindow(win);
  if (window)
  {
    XWindowChanges &wc = window->saveWc();
    geo.x = wc.x;
    geo.y = wc.y;
    geo.width = wc.width;
    geo.height = wc.height;
  }

  return geo;
}

nux::Geometry
PluginAdapter::GetScreenGeometry() const
{
  nux::Geometry geo;

  geo.x = 0;
  geo.y = 0;
  geo.width = m_Screen->width();
  geo.height = m_Screen->height();

  return geo;
}

nux::Geometry
PluginAdapter::GetWorkAreaGeometry(guint32 xid) const
{
  CompWindow* window = nullptr;
  unsigned int output = 0;

  if (xid != 0)
  {
    Window win = xid;

    window = m_Screen->findWindow(win);
    if (window)
    {
      output = window->outputDevice();
    }
  }

  if (xid == 0 || !window)
  {
    output = m_Screen->currentOutputDev().id();
  }

  CompRect workarea = m_Screen->getWorkareaForOutput(output);

  return nux::Geometry(workarea.x(), workarea.y(), workarea.width(), workarea.height());
}

bool
PluginAdapter::CheckWindowIntersection(nux::Geometry const& region, CompWindow* window) const
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

void
PluginAdapter::CheckWindowIntersections (nux::Geometry const& region, bool &active, bool &any)
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

int
PluginAdapter::WorkspaceCount() const
{
  return m_Screen->vpSize().width() * m_Screen->vpSize().height();
}

void
PluginAdapter::SetMwmWindowHints(Window xid, MotifWmHints* new_hints)
{
  Display* display = m_Screen->dpy();
  Atom hints_atom = None;
  MotifWmHints* data = NULL;
  MotifWmHints* hints = NULL;
  Atom type = None;
  gint format;
  gulong nitems;
  gulong bytes_after;

  hints_atom = XInternAtom(display, _XA_MOTIF_WM_HINTS, false);

  if (XGetWindowProperty(display,
                         xid,
                         hints_atom, 0, sizeof(MotifWmHints) / sizeof(long),
                         False, AnyPropertyType, &type, &format, &nitems,
                         &bytes_after, (guchar**)&data) != Success)
  {
    return;
  }

  if (type != hints_atom || !data)
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

  XChangeProperty(display,
                  xid,
                  hints_atom, hints_atom, 32, PropModeReplace,
                  (guchar*)hints, sizeof(MotifWmHints) / sizeof(long));

  if (data)
    XFree(data);
}

void
PluginAdapter::Decorate(guint32 xid)
{
  MotifWmHints hints = { 0 };

  hints.flags = MWM_HINTS_DECORATIONS;
  hints.decorations = GDK_DECOR_ALL & ~(MWM_HINTS_UNDECORATED_UNITY);

  SetMwmWindowHints(xid, &hints);
}

void
PluginAdapter::Undecorate(guint32 xid)
{
  MotifWmHints hints = { 0 };

  /* Set the high bit to indicate that we undecorated this
   * window, when an application attempts to "undecorate"
   * the window again, this bit will be cleared */
  hints.flags = MWM_HINTS_DECORATIONS;
  hints.decorations = MWM_HINTS_UNDECORATED_UNITY;

  SetMwmWindowHints(xid, &hints);
}

bool
PluginAdapter::IsScreenGrabbed() const
{
  return m_Screen->grabbed();
}

bool
PluginAdapter::IsViewPortSwitchStarted() const
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

void
PluginAdapter::ShowGrabHandles(CompWindow* window, bool use_timer)
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

void
PluginAdapter::HideGrabHandles(CompWindow* window)
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

void
PluginAdapter::ToggleGrabHandles(CompWindow* window)
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

void
PluginAdapter::SetCoverageAreaBeforeAutomaximize(float area)
{
  _coverage_area_before_automaximize = area;
}

bool
PluginAdapter::saveInputFocus()
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

bool
PluginAdapter::restoreInputFocus()
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

void
PluginAdapter::MoveResizeWindow(guint32 xid, nux::Geometry geometry)
{
  int w, h;
  CompWindow* window = m_Screen->findWindow(xid);

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

void
PluginAdapter::OnWindowClosed(CompWindow *w)
{
  if (_last_focused_window == w)
    _last_focused_window = NULL;
}

void
PluginAdapter::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper wrapper(builder);
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

std::string
PluginAdapter::GetWindowName(guint32 xid) const
{
  std::string name;
  Atom visibleNameAtom;

  visibleNameAtom = XInternAtom(m_Screen->dpy(), "_NET_WM_VISIBLE_NAME", 0);
  name = GetUtf8Property(xid, visibleNameAtom);
  if (name.empty())
  {
    Atom wmNameAtom = XInternAtom(m_Screen->dpy(), "_NET_WM_NAME", 0);
    name = GetUtf8Property(xid, wmNameAtom);
  }

  if (name.empty())
    name = GetTextProperty(xid, XA_WM_NAME);

  return name;
}

std::string
PluginAdapter::GetUtf8Property(guint32 xid, Atom atom) const
{
  Atom          type;
  int           result, format;
  unsigned long nItems, bytesAfter;
  char          *val;
  std::string   retval;
  Atom          utf8StringAtom;

  utf8StringAtom = XInternAtom(m_Screen->dpy(), "UTF8_STRING", 0);
  result = XGetWindowProperty(m_Screen->dpy(), xid, atom, 0L, 65536, False,
                              utf8StringAtom, &type, &format, &nItems,
                              &bytesAfter, reinterpret_cast<unsigned char **>(&val));

  if (result != Success)
    return retval;

  if (type == utf8StringAtom && format == 8 && val && nItems > 0)
  {
    retval = std::string(val, nItems);
  }
  if (val)
    XFree(val);

  return retval;
}

std::string
PluginAdapter::GetTextProperty(guint32 id, Atom atom) const
{
  XTextProperty text;
  std::string retval;

  text.nitems = 0;
  if (XGetTextProperty(m_Screen->dpy(), id, &text, atom))
  {
    if (text.value)
    {
      retval = std::string(reinterpret_cast<char*>(text.value), text.nitems);
      XFree (text.value);
    }
  }

  return retval;
}
