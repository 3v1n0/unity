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
#include "PluginAdapter.h"

PluginAdapter * PluginAdapter::_default = 0;

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
PluginAdapter::NotifyStateChange (CompWindow *window, unsigned int state, unsigned int last_state)
{
  if (!(last_state & MAXIMIZE_STATE) && (state & MAXIMIZE_STATE))
    window_maximized.emit (window);
  else if ((last_state & MAXIMIZE_STATE) && !(state & MAXIMIZE_STATE))
    window_restored.emit (window);
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
      window_mapped.emit (window);
      break;
    case CompWindowNotifyUnmap:
      window_unmapped.emit (window);
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
    return window->state () & MAXIMIZE_STATE;
  }

  return false;
}
