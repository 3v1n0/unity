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

#include "PlaceEntryRemote.h"

PlaceEntryRemote::PlaceEntryRemote (Place *parent)
: _parent (parent),
  _name (NULL),
  _icon (NULL),
  _description (NULL),
  _position (0),
  _mimetypes (NULL),
  _sensitive (false),
  _active (false),
  _valid (false)
{
}

PlaceEntryRemote::~PlaceEntryRemote ()
{
  g_free (_name);
  g_free (_icon);
  g_free (_description);
  g_strfreev (_mimetypes);
}

void
PlaceEntryRemote::InitFromKeyFile (GKeyFile    *key_file,
                                   const gchar *group)
{

  _valid = true;
}

/* Other methods */
bool
PlaceEntryRemote::IsValid ()
{
  return _valid;
}

/* Overrides */
const gchar *
PlaceEntryRemote::GetName ()
{
  return _name;
}

const gchar *
PlaceEntryRemote::GetIcon ()
{
  return _icon;
}

const gchar *
PlaceEntryRemote::GetDescription ()
{
  return _description;
}

const guint32
PlaceEntryRemote::GetPosition  ()
{
  return _position;
}

const gchar **
PlaceEntryRemote::GetMimetypes ()
{
  return (const gchar **)_mimetypes;
}

const std::map<gchar *, gchar *>&
PlaceEntryRemote::GetHints ()
{
  return _hints;
}

const bool
PlaceEntryRemote::IsSensitive ()
{
  return _sensitive;
}

const bool
PlaceEntryRemote::IsActive ()
{
  return _active;
}

Place *
PlaceEntryRemote::GetParent ()
{
  return _parent;
}
