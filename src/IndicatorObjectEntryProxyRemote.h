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

#ifndef INDICATOR_OBJECT_ENTRY_PROXY_REMOTE_H
#define INDICATOR_OBJECT_ENTRY_PROXY_REMOTE_H

#include <string>
#include <gio/gio.h>
#include <dee.h>

#include "IndicatorObjectEntryProxy.h"

// Represents an IndicatorObjectEntry over DBus through the panel service
// Basically wraps the DeeModelIter, trying to keep as much info inside 
// that as possible to avoid duplication

class IndicatorObjectEntryProxyRemote : public IndicatorObjectEntryProxy
{
public:

  IndicatorObjectEntryProxyRemote  (DeeModel *model, DeeModelIter *iter);
  ~IndicatorObjectEntryProxyRemote ();

  const char * GetId ();

  virtual const char * GetLabel ();

  // Call g_object_unref on the returned pixbuf
  virtual GdkPixbuf  * GetPixbuf ();

  virtual void         SetActive (bool active);
  virtual bool         GetActive ();

  virtual void         ShowMenu (int x, int y, guint32 timestamp);

  void Refresh ();

  // Signals
  sigc::signal<void, const char *, int, int, guint32> OnShowMenuRequest;

public:
  DeeModel     *_model;
  DeeModelIter *_iter;
  bool          _active;
};

#endif // INDICATOR_OBJECT_ENTRY_PROXY_REMOTE_H
