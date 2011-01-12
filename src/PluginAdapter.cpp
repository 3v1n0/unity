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
#include <scale/scale.h>
#include "PluginAdapter.h"

PluginAdapter * PluginAdapter::_default = 0;

#define MAXIMIZABLE (CompWindowActionMaximizeHorzMask & CompWindowActionMaximizeVertMask & CompWindowActionResizeMask)

#define nb_default_exclude_wmclasses 18
static const char *default_exclude_wmclasses[nb_default_exclude_wmclasses] =
    {
      "Apport-gtk",
      "Bluetooth-properties",
      "Bluetooth-wizard",
      "Download", /* Firefox Download Window */
      "Ekiga",
      "Extension", /* Firefox Add-Ons/Extension Window */
      "Gimp",
      "Global", /* Firefox Error Console Window */
      "Gnome-nettool",
      "Kiten",
      "Kmplot",
      "Nm-editor",
      "Pidgin",
      "Polkit-gnome-authorization",
      "Update-manager",
      "Skype",
      "Toplevel", /* Firefox "Clear Private Data" Window */
      "Transmission"
    };


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

PluginAdapter::PluginAdapter(CompScreen *screen)
{
  m_Screen = screen;
  m_ExpoAction = 0;
  m_ScaleAction = 0;
}

PluginAdapter::~PluginAdapter()
{
}

void 
PluginAdapter::NotifyResized (CompWindow *window, int x, int y, int w, int h)
{
  window_resized.emit (window);
}

void 
PluginAdapter::NotifyMoved (CompWindow *window, int x, int y)
{
  window_moved.emit (window);
}

void
PluginAdapter::NotifyStateChange (CompWindow *window, unsigned int state, unsigned int last_state)
{
  if (!((last_state & MAXIMIZE_STATE) == MAXIMIZE_STATE)
      && ((state & MAXIMIZE_STATE) == MAXIMIZE_STATE))
  {
    PluginAdapter::window_maximized.emit (window);
    WindowManager::window_maximized.emit (window->id ());
  }
  else if (((last_state & MAXIMIZE_STATE) == MAXIMIZE_STATE)
           && !((state & MAXIMIZE_STATE) == MAXIMIZE_STATE))
  {
    PluginAdapter::window_restored.emit (window);
    WindowManager::window_restored.emit (window->id ());
  }
}

void 
PluginAdapter::Notify (CompWindow *window, CompWindowNotify notify)
{
  switch (notify)
  {
    case CompWindowNotifyMinimize:
      window_minimized.emit (window);
      break;
    case CompWindowNotifyUnminimize:
      window_unminimized.emit (window);
      break;
    case CompWindowNotifyShade:
      window_shaded.emit (window);
      break;
    case CompWindowNotifyUnshade:
      window_unshaded.emit (window);
      break;
    case CompWindowNotifyHide:
      window_hidden.emit (window);
      break;
    case CompWindowNotifyShow:
      window_shown.emit (window);
      break;
    case CompWindowNotifyMap:
      PluginAdapter::window_mapped.emit (window);
      WindowManager::window_mapped.emit (window->id ());
      break;
    case CompWindowNotifyUnmap:
      PluginAdapter::window_unmapped.emit (window);
      WindowManager::window_unmapped.emit (window->id ());
      break;
    case CompWindowNotifyReparent:
      MaximizeIfBigEnough (window);
      break;
    default:
      break;
  }
}

void
PluginAdapter::SetExpoAction (CompAction *expo)
{
    m_ExpoAction = expo;
}

void
PluginAdapter::SetScaleAction (CompAction *scale)
{
    m_ScaleAction = scale;
}
    
std::string *
PluginAdapter::MatchStringForXids (std::list<Window> *windows)
{
    char *string;
    std::string *result = new std::string ("any & (");
    
    std::list<Window>::iterator it;
    
    for (it = windows->begin (); it != windows->end (); it++)
    {
        string = g_strdup_printf ("| xid=%i ", (int) *it);
        result->append (string);
        g_free (string);
    }
    
    result->append (")");
    
    return result;
}
    
void 
PluginAdapter::InitiateScale (std::string *match)
{
    if (!m_ScaleAction)
        return;
        
    CompOption::Value value;
    CompOption::Type  type;
    CompOption::Vector argument;
    char             *name;

    name = (char *) "root";
    type = CompOption::TypeInt;
    value.set ((int) m_Screen->root ());
    
    CompOption arg = CompOption (name, type);
    arg.set (value);
    argument.push_back (arg);
    
    name = (char *) "match";
    type = CompOption::TypeMatch;
    value.set (CompMatch (*match));
    
    arg = CompOption (name, type);
    arg.set (value);
    argument.push_back (arg);
    
    m_ScaleAction->initiate () (m_ScaleAction, 0, argument);
}
    
bool 
PluginAdapter::IsScaleActive ()
{
    SCALE_SCREEN(m_Screen);
    return (m_ScaleAction && ss && ss->hasGrab ());
}

void 
PluginAdapter::TerminateScale ()
{
    if (!IsScaleActive ())
        return;

    CompOption::Value value;
    CompOption::Type  type;
    CompOption::Vector argument;
    char             *name;

    name = (char *) "root";
    type = CompOption::TypeInt;
    value.set ((int) m_Screen->root ());
    
    CompOption arg = CompOption (name, type);
    arg.set (value);
    argument.push_back (arg);
    
    m_ScaleAction->terminate () (m_ScaleAction, 0, argument);
}

void 
PluginAdapter::InitiateExpo ()
{
    if (!m_ExpoAction)
        return;
        
    CompOption::Value value;
    CompOption::Type  type;
    CompOption::Vector argument;
    char             *name;

    name = (char *) "root";
    type = CompOption::TypeInt;
    value.set ((int) m_Screen->root ());
    
    CompOption arg (name, type);
    arg.set (value);
    argument.push_back (arg);
    
    m_ExpoAction->initiate () (m_ExpoAction, 0, argument);
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

void PluginAdapter::MaximizeIfBigEnough (CompWindow *window)
{
  XClassHint   classHint;
  Status       status;
  char*        win_wmclass = NULL;
  int          num_monitor;
  CompOutput   screen;
  int          screen_width;
  int          screen_height;

  if (!window)
    return;

  if (window->type () != CompWindowTypeNormalMask
      || (window->actions () & MAXIMIZABLE) != MAXIMIZABLE)
    return;

  status = XGetClassHint (m_Screen->dpy (), window->id (), &classHint);
  if (status && classHint.res_class)
  {
    win_wmclass = strdup (classHint.res_class);
    XFree (classHint.res_class);
  }
  else
    return;

  for(int i = 0; i < nb_default_exclude_wmclasses; i++)
  {
    if (!strcmp (win_wmclass, default_exclude_wmclasses[i]))
    {
      g_debug ("MaximizeIfBigEnough: Blacklisted app detected: %s", win_wmclass);
      return;
    }
  }

  num_monitor = window->outputDevice();
  screen = m_Screen->outputDevs().at(num_monitor);

  screen_height = screen.workArea().height ();
  screen_width = screen.workArea().width ();

  if ((window->state () & MAXIMIZE_STATE) == MAXIMIZE_STATE) {
    g_debug ("MaximizeIfBigEnough: window mapped and already maximized, just undecorate");
    Undecorate (window->id ());
    return;
  }

  // use server<parameter> because the window won't show the real parameter as
  // not mapped yet
  if ((window->serverWidth () < (screen_width * 0.6)) || (window->serverWidth () > screen_width)
       || (window->serverHeight () < (screen_height * 0.6)) || (window->serverHeight () > screen_height)
       || (((float)window->serverWidth () / (float)window->serverHeight ()) < 0.6)
       || (((float)window->serverWidth () / (float)window->serverHeight ()) > 2.0))
  {
    g_debug ("MaximizeIfBigEnough: %s window size doesn't fit", win_wmclass);
    return;
  }

  Undecorate (window->id ());
  window->maximize (MAXIMIZE_STATE);

  if (win_wmclass)
    free (win_wmclass);
}
