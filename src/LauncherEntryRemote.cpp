// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 */

#include "LauncherEntryRemote.h"

/**
 * Create a new LauncherEntryRemote parsed from the raw DBus wire format
 * of the com.canonical.Unity.LauncherEntry.Update signal '(sa{sv})'.
 *
 * You don't normally need to use this constructor yourself. The
 * LauncherEntryRemoteModel will do that for you when needed.
 */
LauncherEntryRemote::LauncherEntryRemote(GVariant *val)
{
  gchar        *app_uri, *prop_key;
  GVariantIter *prop_iter;
  GVariant     *prop_value;

  g_return_if_fail (val != NULL);

  _emblem = NULL;
  _count = G_GINT64_CONSTANT (0);
  _progress = 0.0;
  
  _emblem_visible = FALSE;
  _count_visible = FALSE;
  _progress_visible = FALSE;

  g_variant_ref_sink (val);
  g_variant_get (val, "(sa{sv})", &app_uri, &prop_iter);

  _app_uri = app_uri; // steal ref

  while (g_variant_iter_loop (prop_iter, "{sv}", &prop_key, &prop_value))
    {
      if (g_str_equal ("emblem", prop_key))
        g_variant_get (prop_value, "s", &_emblem);
      else if (g_str_equal ("count", prop_key))
        _count = g_variant_get_int64 (prop_value);
      else if (g_str_equal ("progress", prop_key))
        _progress = g_variant_get_double (prop_value);
      else if (g_str_equal ("emblem-visible", prop_key))
        _emblem_visible = g_variant_get_boolean (prop_value);
      else if (g_str_equal ("count-visible", prop_key))
        _count_visible = g_variant_get_boolean (prop_value);
      else if (g_str_equal ("progress-visible", prop_key))
        _progress_visible = g_variant_get_boolean (prop_value);
    }

  g_variant_iter_free (prop_iter);
  g_variant_unref (val);
}

LauncherEntryRemote::~LauncherEntryRemote()
{
  if (_app_uri)
    {
      g_free (_app_uri);
      _app_uri = NULL;
    }
  
  if (_emblem)
    {
      g_free (_emblem);
      _emblem = NULL;
    }
}

const gchar*
LauncherEntryRemote::AppUri()
{
  return _app_uri;
}


const gchar*
LauncherEntryRemote::Emblem()
{
  return _emblem;
}

gint64
LauncherEntryRemote::Count()
{
  return _count;
}

gdouble
LauncherEntryRemote::Progress()
{
  return _progress;
}

gboolean
LauncherEntryRemote::EmblemVisible()
{
  return _emblem_visible;
}

gboolean
LauncherEntryRemote::CountVisible()
{
  return _count_visible;
}

gboolean
LauncherEntryRemote::ProgressVisible()
{
  return _progress_visible;
}

void
LauncherEntryRemote::SetEmblem(const gchar* emblem)
{
  if (g_strcmp0 (_emblem, emblem) == 0)
    return;

  if (_emblem)
    g_free (_emblem);

  _emblem = g_strdup (emblem);
  emblem_changed.emit ();
}

void
LauncherEntryRemote::SetCount(gint64 count)
{
  if (_count == count)
    return;

  _count = count;
  count_changed.emit ();
}

void
LauncherEntryRemote::SetProgress(gdouble progress)
{
  if (_progress == progress)
    return;

  _progress = progress;
  progress_changed.emit ();
}

void
LauncherEntryRemote::SetEmblemVisible(gboolean visible)
{
  if (_emblem_visible == visible)
    return;

  _emblem_visible = visible;
  emblem_visible_changed.emit ();
}

void
LauncherEntryRemote::SetCountVisible(gboolean visible)
{
  if (_count_visible == visible)
      return;

  _count_visible = visible;
  count_visible_changed.emit ();
}

void
LauncherEntryRemote::SetProgressVisible(gboolean visible)
{
  if (_progress_visible == visible)
      return;

  _progress_visible = visible;
  progress_visible_changed.emit ();
}

/**
 * Set all properties from 'other' on 'this'.
 */
void
LauncherEntryRemote::Update(LauncherEntryRemote *other)
{
  SetEmblem (other->Emblem ());
  SetCount (other->Count ());
  SetProgress (other->Progress ());

  SetEmblemVisible (other->EmblemVisible ());
  SetCountVisible(other->CountVisible ());
  SetProgressVisible(other->ProgressVisible());
}
