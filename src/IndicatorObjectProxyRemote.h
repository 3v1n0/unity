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

#ifndef INDICATOR_OBJECT_PROXY_REMOTE_H
#define INDICATOR_OBJECT_PROXY_REMOTE_H

#include <string>
#include <gio/gio.h>
#include <dee.h>

#include "IndicatorObjectProxy.h"

// Represents an IndicatorObject over DBus through the panel service

class IndicatorObjectProxyRemote : public IndicatorObjectProxy
{
public:

  IndicatorObjectProxyRemote  (const char *name, const char *model_name);
  ~IndicatorObjectProxyRemote ();
  
  virtual std::string& GetName ();
  virtual std::vector<IndicatorObjectEntryProxy *>& GetEntries ();

  void OnRowAdded   (DeeModelIter *iter);
  void OnRowChanged (DeeModelIter *iter);
  void OnRowRemoved (DeeModelIter *iter);
  void OnShowMenuRequestReceived (const char *id, int x, int y, guint timestamp);

  // Signals
  sigc::signal<void, const char *, int, int, guint32> OnShowMenuRequest;

private:
  std::string _name;
  std::string _model_name;
  DeeModel    *_model;
};

#endif // INDICATOR_OBJECT_PROXY_REMOTE_H
