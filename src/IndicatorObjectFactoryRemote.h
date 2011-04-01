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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef INDICATOR_OBJECT_FACTORY_REMOTE_H
#define INDICATOR_OBJECT_FACTORY_REMOTE_H

#include <string>
#include <gio/gio.h>
#include <dee.h>

#include "IndicatorObjectFactory.h"
#include "IndicatorObjectProxyRemote.h"

// Connects to the remote panel service (unity-panel-service) and translates
// that into something that the panel can show
class IndicatorObjectFactoryRemote : public IndicatorObjectFactory
{
public:

  IndicatorObjectFactoryRemote  ();
  ~IndicatorObjectFactoryRemote ();
  
  virtual std::vector<IndicatorObjectProxy *> &GetIndicatorObjects ();
  virtual void ForceRefresh ();

  void OnRemoteProxyReady (GDBusProxy *proxy);
  void OnEntryActivated   (const char *entry_id);
  void OnShowMenuRequestReceived (const char *id, int x, int y, guint timestamp, guint32 button);
  void OnScrollReceived (const char *id, int delta);
  void Sync (GVariant *args);
  void OnEntryActivateRequestReceived (const char *entry_id);
  void Reconnect ();
  void OnEntryShowNowChanged (const char *entry_id, bool show_now_state);

  void AddProperties (GVariantBuilder *builder);

  GDBusProxy * GetRemoteProxy ();

private:
  IndicatorObjectProxyRemote* IndicatorForID (const char *id);
private:
  GDBusProxy *_proxy;
};

#endif // INDICATOR_OBJECT_FACTORY_REMOTE_H
