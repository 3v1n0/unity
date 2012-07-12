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

//FIXME!! Entirely stubs for now, unless we need this functionality at some point

namespace
{
nux::logging::Logger logger("unity.plugin");
}

PluginAdapter* PluginAdapter::_default = 0;

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
  m_ExpoActionList(0),
  m_ScaleActionList(0),
  _in_show_desktop (false),
  _last_focused_window(nullptr)
{
}

PluginAdapter::~PluginAdapter()
{
}

/* A No-op for now, but could be useful later */
void
PluginAdapter::OnScreenGrabbed()
{
}

void
PluginAdapter::OnScreenUngrabbed()
{
}

void
PluginAdapter::NotifyResized(CompWindow* window, int x, int y, int w, int h)
{
}

void
PluginAdapter::NotifyMoved(CompWindow* window, int x, int y)
{
}

void
PluginAdapter::NotifyStateChange(CompWindow* window, unsigned int state, unsigned int last_state)
{
}

void
PluginAdapter::NotifyNewDecorationState(guint32 xid)
{
}

void
PluginAdapter::Notify(CompWindow* window, CompWindowNotify notify)
{
}

void
PluginAdapter::NotifyCompizEvent(const char* plugin, const char* event, CompOption::Vector& option)
{
}

void
MultiActionList::AddNewAction(CompAction* a, bool primary)
{
}

void
MultiActionList::RemoveAction(CompAction* a)
{
}

void
MultiActionList::InitiateAll(CompOption::Vector& extraArgs, int state)
{
}

void
MultiActionList::TerminateAll(CompOption::Vector& extraArgs)
{
}

unsigned long long
PluginAdapter::GetWindowActiveNumber (guint32 xid)
{
  return 0;
}

void
PluginAdapter::SetExpoAction(MultiActionList& expo)
{
}

void
PluginAdapter::SetScaleAction(MultiActionList& scale)
{
}

std::string
PluginAdapter::MatchStringForXids(std::vector<Window> *windows)
{
  return "";
}

void
PluginAdapter::InitiateScale(std::string const& match, int state)
{
}

void
PluginAdapter::TerminateScale()
{
}

bool
PluginAdapter::IsScaleActive()
{
  return false;
}

bool
PluginAdapter::IsScaleActiveForGroup()
{
  return false;
}

bool
PluginAdapter::IsExpoActive()
{
  return false;
}

void
PluginAdapter::InitiateExpo()
{
}

// WindowManager implementation
guint32
PluginAdapter::GetActiveWindow()
{
  return 0;
}

bool
PluginAdapter::IsWindowMaximized(guint xid)
{
  return false;
}

bool
PluginAdapter::IsWindowDecorated(guint32 xid)
{
  return false;
}

bool
PluginAdapter::IsWindowOnCurrentDesktop(guint32 xid)
{
  return false;
}

bool
PluginAdapter::IsWindowObscured(guint32 xid)
{
  return false;
}

bool
PluginAdapter::IsWindowMapped(guint32 xid)
{
  return false;
}

bool
PluginAdapter::IsWindowVisible(guint32 xid)
{
  return false;
}

bool
PluginAdapter::IsWindowOnTop(guint32 xid)
{
  return false;
}

bool
PluginAdapter::IsWindowClosable(guint32 xid)
{
  return false;
}

bool
PluginAdapter::IsWindowMinimizable(guint32 xid)
{
  return false;
}

bool
PluginAdapter::IsWindowMaximizable(guint32 xid)
{
  return false;
}

void
PluginAdapter::Restore(guint32 xid)
{
}

void
PluginAdapter::RestoreAt(guint32 xid, int x, int y)
{
}

void
PluginAdapter::Minimize(guint32 xid)
{
}

void
PluginAdapter::Close(guint32 xid)
{
  GdkDisplay* gdkdisplay;
  GdkScreen* screen;
  Display* xdisplay;
  Atom net_close_win;
  Window xroot;
  XEvent ev;

  gdkdisplay  = gdk_display_get_default();
  xdisplay    = GDK_DISPLAY_XDISPLAY(gdkdisplay);
  screen      = gdk_display_get_default_screen (gdkdisplay);
  xroot       = RootWindowOfScreen (gdk_x11_screen_get_xscreen (screen));

  net_close_win = XInternAtom (xdisplay, "_NET_CLOSE_WINDOW", 0);
  printf("Here\n");

  ev.xclient.type         = ClientMessage;
  ev.xclient.display      = xdisplay;

  ev.xclient.serial       = 0;
  ev.xclient.send_event   = TRUE;

  ev.xclient.window       = xid;
  ev.xclient.message_type = net_close_win;
  ev.xclient.format       = 32;

  ev.xclient.data.l[0]    = CurrentTime;
  ev.xclient.data.l[1]    = 1; //application

  XSendEvent (xdisplay, xroot, FALSE,
    SubstructureRedirectMask | SubstructureNotifyMask,
    &ev);

  XSync (xdisplay, FALSE);
}

void
PluginAdapter::Activate(guint32 xid)
{
}

void
PluginAdapter::Raise(guint32 xid)
{
}

void
PluginAdapter::Lower(guint32 xid)
{
}

void
PluginAdapter::FocusWindowGroup(std::vector<Window> window_ids, FocusVisibility focus_visibility, int monitor, bool only_top_win)
{
}

bool
PluginAdapter::ScaleWindowGroup(std::vector<Window> windows, int state, bool force)
{
  return false;
}

void
PluginAdapter::SetWindowIconGeometry(Window window, nux::Geometry const& geo)
{
}

void
PluginAdapter::ShowDesktop()
{
}

void
PluginAdapter::OnShowDesktop()
{
}

void
PluginAdapter::OnLeaveDesktop()
{
}

int
PluginAdapter::GetWindowMonitor(guint32 xid) const
{
  return -1;
}

nux::Geometry
PluginAdapter::GetWindowGeometry(guint32 xid) const
{
  nux::Geometry geo(0, 0, 1, 1);
  return geo;
}

nux::Geometry
PluginAdapter::GetWindowSavedGeometry(guint32 xid) const
{
  nux::Geometry geo(0, 0, 1, 1);
  return geo;
}

nux::Geometry
PluginAdapter::GetScreenGeometry() const
{
  nux::Geometry geo(0, 0, 1, 1);
  return geo;
}

nux::Geometry
PluginAdapter::GetWorkAreaGeometry(guint32 xid) const
{
  nux::Geometry geo(0, 0, 1, 1);
  return geo;
}

bool
PluginAdapter::CheckWindowIntersection(nux::Geometry const& region, CompWindow* window)
{
  return false;
}

void
PluginAdapter::CheckWindowIntersections (nux::Geometry const& region, bool &active, bool &any)
{
}

int
PluginAdapter::WorkspaceCount()
{
  return 4;
}

void
PluginAdapter::SetMwmWindowHints(Window xid, MotifWmHints* new_hints)
{
}

void
PluginAdapter::Decorate(guint32 xid)
{
}

void
PluginAdapter::Undecorate(guint32 xid)
{
}

bool
PluginAdapter::IsScreenGrabbed()
{
  return false;
}

bool
PluginAdapter::IsViewPortSwitchStarted()
{
  return false;
}

/* Returns true if the window was maximized */
bool PluginAdapter::MaximizeIfBigEnough(CompWindow* window)
{
  return true;
}

void
PluginAdapter::ShowGrabHandles(CompWindow* window, bool use_timer)
{
}

void
PluginAdapter::HideGrabHandles(CompWindow* window)
{
}

void
PluginAdapter::ToggleGrabHandles(CompWindow* window)
{
}

void
PluginAdapter::SetCoverageAreaBeforeAutomaximize(float area)
{
}

bool
PluginAdapter::saveInputFocus()
{
  return false;
}

bool
PluginAdapter::restoreInputFocus()
{
  return false;
}

void
PluginAdapter::MoveResizeWindow(guint32 xid, nux::Geometry geometry)
{
}

void
PluginAdapter::OnWindowClosed(CompWindow *w)
{
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
