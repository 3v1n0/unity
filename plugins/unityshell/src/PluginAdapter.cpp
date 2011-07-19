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

#include "NuxCore/Logger.h"

namespace {

nux::logging::Logger logger("unity.plugin");

}

PluginAdapter * PluginAdapter::_default = 0;

#define MAXIMIZABLE (CompWindowActionMaximizeHorzMask & CompWindowActionMaximizeVertMask & CompWindowActionResizeMask)

#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define _XA_MOTIF_WM_HINTS		"_MOTIF_WM_HINTS"

/* static */
PluginAdapter *
PluginAdapter::Default ()
{
  if (!_default)
      return 0;
  return _default;
}

/* static */
void
PluginAdapter::Initialize (CompScreen *screen)
{
  _default = new PluginAdapter (screen);
}

PluginAdapter::PluginAdapter(CompScreen *screen) :
    m_Screen (screen),
    m_ExpoActionList (0),
    m_ScaleActionList (0)
{
  _spread_state = false;
  _expo_state = false;
  
  _grab_show_action = 0;
  _grab_hide_action = 0;
  _grab_toggle_action = 0;
  _coverage_area_before_automaximize = 0.75;
}

PluginAdapter::~PluginAdapter()
{
}

/* A No-op for now, but could be useful later */
void
PluginAdapter::OnScreenGrabbed ()
{
  compiz_screen_grabbed.emit ();

  if (!_spread_state && screen->grabExist ("scale"))
  {
    _spread_state = true;
    initiate_spread.emit ();
  }
  
  if (!_expo_state && screen->grabExist ("expo"))
  {
    _expo_state = true;
    initiate_expo.emit ();
  }
}

void
PluginAdapter::OnScreenUngrabbed ()
{
  if (_spread_state && !screen->grabExist ("scale"))
  {
    // restore windows to their pre-spread minimized state
    for (std::list<guint32>::iterator itr = m_SpreadedWindows.begin (); itr != m_SpreadedWindows.end (); ++itr)
    {
      if (*itr != m_Screen->activeWindow ())
      {
        CompWindow* window = m_Screen->findWindow (*itr);
        if (window)
          window->minimize ();
      }
    }
    m_SpreadedWindows.clear ();
    _spread_state = false;
    terminate_spread.emit ();
  }
  
  if (_expo_state && !screen->grabExist ("expo"))
  {
    _expo_state = false;
    terminate_expo.emit ();
  }
  
  compiz_screen_ungrabbed.emit ();
}

void
PluginAdapter::NotifyResized (CompWindow *window, int x, int y, int w, int h)
{
  window_resized.emit (window->id ());
}

void 
PluginAdapter::NotifyMoved (CompWindow *window, int x, int y)
{
  window_moved.emit (window->id ());
}

void
PluginAdapter::NotifyStateChange (CompWindow *window, unsigned int state, unsigned int last_state)
{
  if (!((last_state & MAXIMIZE_STATE) == MAXIMIZE_STATE)
      && ((state & MAXIMIZE_STATE) == MAXIMIZE_STATE))
  {
    WindowManager::window_maximized.emit (window->id ());
  }
  else if (((last_state & MAXIMIZE_STATE) == MAXIMIZE_STATE)
           && !((state & MAXIMIZE_STATE) == MAXIMIZE_STATE))
  {
    WindowManager::window_restored.emit (window->id ());
  }
}

void 
PluginAdapter::Notify (CompWindow *window, CompWindowNotify notify)
{
  switch (notify)
  {
    case CompWindowNotifyMinimize:
      window_minimized.emit (window->id ());
      break;
    case CompWindowNotifyUnminimize:
      window_unminimized.emit (window->id ());
      break;
    case CompWindowNotifyShade:
      window_shaded.emit (window->id ());
      break;
    case CompWindowNotifyUnshade:
      window_unshaded.emit (window->id ());
      break;
    case CompWindowNotifyHide:
      window_hidden.emit (window->id ());
      break;
    case CompWindowNotifyShow:
      window_shown.emit (window->id ());
      break;
    case CompWindowNotifyMap:
      WindowManager::window_mapped.emit (window->id ());
      break;
    case CompWindowNotifyUnmap:
      WindowManager::window_unmapped.emit (window->id ());
      break;
    case CompWindowNotifyReparent:
      MaximizeIfBigEnough (window);
      break;
    case CompWindowNotifyFocusChange:
      WindowManager::window_focus_changed.emit (window->id ());
      break;
    default:
      break;
  }
}

void
MultiActionList::AddNewAction (CompAction *a, bool primary)
{
  if (std::find (m_ActionList.begin (), m_ActionList.end (), a)  == m_ActionList.end ())
    m_ActionList.push_back (a);

  if (primary)
    _primary_action = a;
}

void
MultiActionList::RemoveAction (CompAction *a)
{
  m_ActionList.remove (a);
}

void
MultiActionList::InitiateAll (CompOption::Vector &extraArgs, int state)
{
  CompOption::Vector argument;
  if (!m_ActionList.size ())
    return;

  argument.resize (1);
  argument[0].setName ("root", CompOption::TypeInt);
  argument[0].value ().set ((int) screen->root ());
  foreach (CompOption &arg, extraArgs)
  {
    argument.push_back (arg);
  }

  CompAction *a;
  
  if (_primary_action)
    a = _primary_action;
  else
    a = m_ActionList.front ();
  
  /* Initiate the first available action with the arguments */
  a->initiate () (a, state, argument);
}

void
MultiActionList::TerminateAll (CompOption::Vector &extraArgs)
{
  CompOption::Vector argument;
  CompOption::Value  value;
  if (!m_ActionList.size ())
    return;

  argument.resize (1);
  argument[0].setName ("root", CompOption::TypeInt);
  argument[0].value ().set ((int) screen->root ());

  foreach (CompOption &a, extraArgs)
    argument.push_back (a);
    
  if (_primary_action)
  {
    _primary_action->terminate () (_primary_action, 0, argument);
    return;
  }
  
  foreach (CompAction *action, m_ActionList)
  {
    if (action->state () & (CompAction::StateTermKey |
                            CompAction::StateTermButton |
                            CompAction::StateTermEdge |
                            CompAction::StateTermEdgeDnd))
    {
      action->terminate () (action, 0, argument);
    }
  }
}

void
PluginAdapter::SetExpoAction (MultiActionList &expo)
{
  m_ExpoActionList = expo;
}

void
PluginAdapter::SetScaleAction (MultiActionList &scale)
{
  m_ScaleActionList = scale;
}

std::string
PluginAdapter::MatchStringForXids (std::list<Window> *windows)
{
  std::ostringstream sout;

  sout << "any & (";

  std::list<Window>::iterator it;
  for (it = windows->begin (); it != windows->end (); ++it)
  {
    sout << "| xid=" << static_cast<int>(*it) << " ";
  }
  sout << ")";

  return sout.str();
}

void
PluginAdapter::InitiateScale (std::string const& match, int state)
{
  CompOption::Vector argument;
  CompMatch	     m (match);

  argument.resize (1);
  argument[0].setName ("match", CompOption::TypeMatch);
  argument[0].value ().set (m);

  /* FIXME: Lame */
  foreach (CompWindow *w, screen->windows ())
  {
    if (m.evaluate (w))
    {
      /* FIXME:
         just unminimize minimized window for now, don't minimize them after the scale if not picked as TerminateScale is only 
         called if you click on the launcher, not on any icon. More generally, we should hook up InitiateScale and TerminateScale
         to a Scale plugin signal as the shortcut will have a different behaviour then.
      */
      if (w->minimized ())
      {
        // keep track of windows that were unminimzed to restore their state after the spread
        if (std::find (m_SpreadedWindows.begin (), m_SpreadedWindows.end (), w->id ()) == m_SpreadedWindows.end ())
          m_SpreadedWindows.push_back (w->id ());
        w->unminimize ();
      }
    }
  }

  m_ScaleActionList.InitiateAll (argument, state);
}

void 
PluginAdapter::TerminateScale ()
{
  CompOption::Vector argument (0);
  m_ScaleActionList.TerminateAll (argument);
}

bool
PluginAdapter::IsScaleActive ()
{
  return m_Screen->grabExist ("scale");
}

bool
PluginAdapter::IsExpoActive ()
{
  return m_Screen->grabExist ("expo");
}

void 
PluginAdapter::InitiateExpo ()
{
    CompOption::Vector argument (0);
    
    m_ExpoActionList.InitiateAll (argument, 0);
}

// WindowManager implementation
bool
PluginAdapter::IsWindowMaximized (guint xid)
{
  Window win = (Window)xid;
  CompWindow *window;

  window = m_Screen->findWindow (win);
  if (window)
  {
    return ((window->state () & MAXIMIZE_STATE) == MAXIMIZE_STATE);
  }

  return false;
}

bool
PluginAdapter::IsWindowDecorated (guint32 xid)
{
  Window win = (Window)xid;
  CompWindow *window;

  window = m_Screen->findWindow (win);
  if (window)
  {
    unsigned int decor = window->mwmDecor ();

    return decor & (MwmDecorAll | MwmDecorTitle);
  }
  return true;
}

bool
PluginAdapter::IsWindowOnCurrentDesktop (guint32 xid)
{
  Window win = (Window)xid;
  CompWindow *window;

  window = m_Screen->findWindow (win);
  if (window)
  {
    // we aren't checking window->onCurrentDesktop (), as the name implies, because that is broken
    return (window->defaultViewport () == m_Screen->vp ());
  }

  return false;
}

bool
PluginAdapter::IsWindowObscured (guint32 xid)
{
  Window win = (Window)xid;
  CompWindow *window;

  window = m_Screen->findWindow (win);
  if (window)
  {
    CompPoint window_vp = window->defaultViewport ();
    // Check if any windows above this one are blocking it
    for (CompWindow *sibling = window->next; sibling != NULL; sibling = sibling->next)
    {
      if (sibling->defaultViewport () == window_vp
          && !sibling->minimized ()
          && (sibling->state () & MAXIMIZE_STATE) == MAXIMIZE_STATE)
        return true;
    }
  }

  return false;
}

void
PluginAdapter::Restore (guint32 xid)
{
  Window win = (Window)xid;
  CompWindow *window;

  window = m_Screen->findWindow (win);
  if (window)
    window->maximize (0);
}

void
PluginAdapter::Minimize (guint32 xid)
{
  Window win = (Window)xid;
  CompWindow *window;

  window = m_Screen->findWindow (win);
  if (window)
    window->minimize ();
}

void
PluginAdapter::Close (guint32 xid)
{
  Window win = (Window)xid;
  CompWindow *window;

  window = m_Screen->findWindow (win);
  if (window)
    window->close (CurrentTime);
}

void
PluginAdapter::Activate (guint32 xid)
{
  Window win = (Window)xid;
  CompWindow *window;

  window = m_Screen->findWindow (win);
  if (window)
    window->activate ();
}

void
PluginAdapter::Raise (guint32 xid)
{
  Window win = (Window)xid;
  CompWindow *window;

  window = m_Screen->findWindow (win);
  if (window)
    window->raise ();
}

void
PluginAdapter::Lower (guint32 xid)
{
  Window win = (Window)xid;
  CompWindow *window;

  window = m_Screen->findWindow (win);
  if (window)
    window->lower ();
}

void
PluginAdapter::ShowDesktop ()
{
  m_Screen->enterShowDesktopMode ();
}

nux::Geometry
PluginAdapter::GetWindowGeometry (guint32 xid)
{
  Window win = (Window)xid;
  CompWindow *window;
  nux::Geometry geo (0, 0, 1, 1);

  window = m_Screen->findWindow (win);
  if (window)
  {
    geo.x = window->x ();
    geo.y = window->y ();
    geo.width = window->width ();
    geo.height = window->height ();
  }
  return geo;
}

void
PluginAdapter::SetMwmWindowHints (Window xid, MotifWmHints *new_hints)
{
  Display *display = m_Screen->dpy ();
  Atom hints_atom = None;
  MotifWmHints *data = NULL;
  MotifWmHints *hints = NULL;
  Atom type = None;
  gint format;
  gulong nitems;
  gulong bytes_after;

  hints_atom = XInternAtom (display, _XA_MOTIF_WM_HINTS, false);

  XGetWindowProperty (display, 
                      xid,
		                  hints_atom, 0, sizeof (MotifWmHints)/sizeof (long),
		                  False, AnyPropertyType, &type, &format, &nitems,
		                  &bytes_after, (guchar **)&data);
    
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
  
  XChangeProperty (display, 
                   xid,
                   hints_atom, hints_atom, 32, PropModeReplace,
                   (guchar *)hints, sizeof (MotifWmHints)/sizeof (long));
  
  if (data)
    XFree (data);
}

void
PluginAdapter::Decorate (guint32 xid)
{
  MotifWmHints hints = { 0 };
    
  hints.flags = MWM_HINTS_DECORATIONS;
  hints.decorations = GDK_DECOR_ALL;
 
  SetMwmWindowHints (xid, &hints);
}

void
PluginAdapter::Undecorate (guint32 xid)
{
  MotifWmHints hints = { 0 };
    
  hints.flags = MWM_HINTS_DECORATIONS;
  hints.decorations = 0;
 
  SetMwmWindowHints (xid, &hints);
}

bool
PluginAdapter::IsScreenGrabbed ()
{
  return m_Screen->grabbed ();
}

void PluginAdapter::MaximizeIfBigEnough (CompWindow *window)
{
  XClassHint   classHint;
  Status       status;
  std::string  win_wmclass;
  int          num_monitor;
  CompOutput   screen;
  int          screen_width;
  int          screen_height;
  float        covering_part;

  if (!window)
    return;

  if (window->type () != CompWindowTypeNormalMask
      || (window->actions () & MAXIMIZABLE) != MAXIMIZABLE)
    return;

  status = XGetClassHint (m_Screen->dpy (), window->id (), &classHint);
  if (status && classHint.res_class)
  {
	win_wmclass = classHint.res_class;
    XFree (classHint.res_class);
  }
  else
    return;

  num_monitor = window->outputDevice();
  screen = m_Screen->outputDevs().at(num_monitor);

  screen_height = screen.workArea().height ();
  screen_width = screen.workArea().width ();

  if ((window->state () & MAXIMIZE_STATE) == MAXIMIZE_STATE) {
    LOG_DEBUG(logger) << "window mapped and already maximized, just undecorate";
    Undecorate (window->id ());
    return;
  }

  // use server<parameter> because the window won't show the real parameter as
  // not mapped yet
  const XSizeHints& hints = window->sizeHints ();
  covering_part = (float)(window->serverWidth () * window->serverHeight ()) / (float)(screen_width * screen_height);
  if ((covering_part < _coverage_area_before_automaximize) || (covering_part > 1.0) ||
      (hints.flags & PMaxSize && (screen_width > hints.max_width || screen_height > hints.max_height)))
  {
    LOG_DEBUG(logger) << win_wmclass << " window size doesn't fit";
    return;
  }

  Undecorate (window->id ());
  window->maximize (MAXIMIZE_STATE);
}

void 
PluginAdapter::ShowGrabHandles (CompWindow *window, bool use_timer)
{
  if (!_grab_show_action || !window)
    return;
    
  CompOption::Vector argument;

  argument.resize (3);
  argument[0].setName ("root", CompOption::TypeInt);
  argument[0].value ().set ((int) screen->root ());
  argument[1].setName ("window", CompOption::TypeInt);
  argument[1].value ().set ((int) window->id ());
  argument[2].setName ("use-timer", CompOption::TypeBool);
  argument[2].value ().set (use_timer);

  /* Initiate the first available action with the arguments */
  _grab_show_action->initiate () (_grab_show_action, 0, argument);
}

void 
PluginAdapter::HideGrabHandles (CompWindow *window)
{
  if (!_grab_hide_action || !window)
    return;
    
  CompOption::Vector argument;

  argument.resize (2);
  argument[0].setName ("root", CompOption::TypeInt);
  argument[0].value ().set ((int) screen->root ());
  argument[1].setName ("window", CompOption::TypeInt);
  argument[1].value ().set ((int) window->id ());

  /* Initiate the first available action with the arguments */
  _grab_hide_action->initiate () (_grab_hide_action, 0, argument);
}

void 
PluginAdapter::ToggleGrabHandles (CompWindow *window)
{
  if (!_grab_toggle_action || !window)
    return;
    
  CompOption::Vector argument;

  argument.resize (2);
  argument[0].setName ("root", CompOption::TypeInt);
  argument[0].value ().set ((int) screen->root ());
  argument[1].setName ("window", CompOption::TypeInt);
  argument[1].value ().set ((int) window->id ());

  /* Initiate the first available action with the arguments */
  _grab_toggle_action->initiate () (_grab_toggle_action, 0, argument);
}

void
PluginAdapter::SetCoverageAreaBeforeAutomaximize (float area) {
  _coverage_area_before_automaximize = area;
}
