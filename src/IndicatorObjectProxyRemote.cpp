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

#include "IndicatorObjectProxyRemote.h"

#include "IndicatorObjectEntryProxyRemote.h"

IndicatorObjectProxyRemote::IndicatorObjectProxyRemote (const char *name)
: _name (name)
{
}

IndicatorObjectProxyRemote::~IndicatorObjectProxyRemote ()
{
  std::vector<IndicatorObjectEntryProxy*>::iterator it;
  
  for (it = _entries.begin(); it != _entries.end(); it++)
  {
    IndicatorObjectEntryProxyRemote *remote = static_cast<IndicatorObjectEntryProxyRemote *> (*it);
    delete remote;
  }

  _entries.erase (_entries.begin (), _entries.end ());
}
  
std::string&
IndicatorObjectProxyRemote::GetName ()
{
  return _name;
}

std::vector<IndicatorObjectEntryProxy *>&
IndicatorObjectProxyRemote::GetEntries ()
{
  return _entries;
}

#if 0
void
IndicatorObjectProxyRemote::OnRowAdded (DeeModelIter *iter)
{
  IndicatorObjectEntryProxyRemote *remote;

  remote = new IndicatorObjectEntryProxyRemote (_model, iter);
  remote->OnShowMenuRequest.connect (sigc::mem_fun (this, &IndicatorObjectProxyRemote::OnShowMenuRequestReceived));
  _entries.push_back (remote);

  OnEntryAdded.emit (remote);
}
#endif

void
IndicatorObjectProxyRemote::OnShowMenuRequestReceived (const char *id,
                                                       int         x,
                                                       int         y,
                                                       guint32     timestamp)
{
  OnShowMenuRequest.emit (id, x, y, timestamp);
}
