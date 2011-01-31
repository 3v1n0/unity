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

LauncherEntryRemote::LauncherEntryRemote()
{
  _emblem = NULL;
  _count = G_GINT64_CONSTANT (0);
  _progress = 0.0;
  
  _emblem_visible = FALSE;
  _count_visible = FALSE;
  _progress_visible = FALSE;
}

LauncherEntryRemote::~LauncherEntryRemote()
{
  
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
  if (_emblem)
    g_free (_emblem);

  _emblem = g_strdup (emblem);
  emblem_changed.emit ();
}

void
LauncherEntryRemote::SetCount(gint64 count)
{
  _count = count;
  count_changed.emit ();
}

void
LauncherEntryRemote::SetProgress(gdouble progress)
{
  _progress = progress;
  progress_changed.emit ();
}

void
LauncherEntryRemote::SetEmblemVisible(gboolean visible)
{
  _emblem_visible = visible;
  emblem_visible_changed.emit ();
}

void
LauncherEntryRemote::SetCountVisible(gboolean visible)
{
  _count_visible = visible;
  count_visible_changed.emit ();
}

void
LauncherEntryRemote::SetProgressVisible(gboolean visible)
{
  _progress_visible = visible;
  progress_visible_changed.emit ();
}
