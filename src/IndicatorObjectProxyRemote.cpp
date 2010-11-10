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

static void on_row_added   (DeeModel                   *model,
                            DeeModelIter               *iter,
                            IndicatorObjectProxyRemote *remote);

static void on_row_changed (DeeModel                   *model,
                            DeeModelIter               *iter,
                            IndicatorObjectProxyRemote *remote);

static void on_row_removed (DeeModel                   *model,
                            DeeModelIter               *iter,
                            IndicatorObjectProxyRemote *remote);

IndicatorObjectProxyRemote::IndicatorObjectProxyRemote (const char *name,
                                                        const char *model_name)
: _name (name),
  _model_name (model_name)
{
  _model = dee_shared_model_new_with_name (model_name);
  g_signal_connect (_model, "row-added",
                    G_CALLBACK (on_row_added), this);
  g_signal_connect (_model, "row-changed",
                    G_CALLBACK (on_row_changed), this);
  g_signal_connect (_model, "row-removed",
                    G_CALLBACK (on_row_removed), this);

  dee_shared_model_connect (DEE_SHARED_MODEL (_model));
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

void
IndicatorObjectProxyRemote::OnRowAdded (DeeModelIter *iter)
{
  IndicatorObjectEntryProxyRemote *remote;

  remote = new IndicatorObjectEntryProxyRemote (_model, iter);
  remote->OnShowMenuRequest.connect (sigc::mem_fun (this, &IndicatorObjectProxyRemote::OnShowMenuRequestReceived));
  _entries.push_back (remote);

  OnEntryAdded.emit (remote);
}

void
IndicatorObjectProxyRemote::OnShowMenuRequestReceived (const char *id,
                                                       int         x,
                                                       int         y,
                                                       guint32     timestamp)
{
  OnShowMenuRequest.emit (id, x, y, timestamp);
}

void
IndicatorObjectProxyRemote::OnRowChanged (DeeModelIter *iter)
{
  std::vector<IndicatorObjectEntryProxy*>::iterator it;
  
  for (it = _entries.begin(); it != _entries.end(); it++)
  {
    IndicatorObjectEntryProxyRemote *remote = static_cast<IndicatorObjectEntryProxyRemote *> (*it);
    if (remote->_iter == iter)
      remote->Refresh ();
  }
}

void
IndicatorObjectProxyRemote::OnRowRemoved (DeeModelIter *iter)
{
  std::vector<IndicatorObjectEntryProxy*>::iterator it;
  
  for (it = _entries.begin(); it != _entries.end(); it++)
  {
    IndicatorObjectEntryProxyRemote *remote = static_cast<IndicatorObjectEntryProxyRemote *> (*it);
    if (remote->_iter == iter)
      {
        _entries.erase (it);
        OnEntryRemoved.emit (remote);
        delete remote;

        break;
      }
  }
}

//
// C callbacks, they just link to class methods and aren't interesting
//
static void
on_row_added (DeeModel *model, DeeModelIter *iter, IndicatorObjectProxyRemote *remote)
{
  remote->OnRowAdded (iter);
}

static void
on_row_changed (DeeModel *model, DeeModelIter *iter, IndicatorObjectProxyRemote *remote)
{
  remote->OnRowChanged (iter);
}

static void
on_row_removed (DeeModel *model, DeeModelIter *iter, IndicatorObjectProxyRemote *remote)
{
  remote->OnRowRemoved (iter);
}
