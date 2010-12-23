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

  IndicatorObjectProxyRemote  (const char *name);
  ~IndicatorObjectProxyRemote ();
  
  virtual std::string& GetName ();
  virtual std::vector<IndicatorObjectEntryProxy *>& GetEntries ();

  void BeginSync ();
  void AddEntry  (const gchar *entry_id,
                  const gchar *label,
                  bool         label_sensitive,
                  bool         label_visible,
                  guint32      image_type,
                  const gchar *image_data,
                  bool         image_sensitive,
                  bool         image_visible);
  void EndSync   ();

  void OnShowMenuRequestReceived (const char *id, int x, int y, guint timestamp, guint32 button);
  void OnScrollReceived (const char *id, int delta);

  // Signals
  sigc::signal<void, const char *, int, int, guint32, guint32> OnShowMenuRequest;
  sigc::signal<void, const char *, int> OnScroll;

private:
  std::string _name;
};

#endif // INDICATOR_OBJECT_PROXY_REMOTE_H
