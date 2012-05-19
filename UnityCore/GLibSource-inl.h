// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2012 Canonical Ltd
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
* Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
*/

#ifndef UNITY_GLIB_SOURCE_INL_H
#define UNITY_GLIB_SOURCE_INL_H

namespace unity
{
namespace glib
{

Timeout::Timeout(unsigned int milliseconds, SourceCallback cb, Priority prio)
  : Source()
{
  Init(milliseconds, prio);
  Run(cb);
}

Timeout::Timeout(unsigned int milliseconds, Priority prio)
  : Source()
{
  Init(milliseconds, prio);
}

void Timeout::Init(unsigned int milliseconds, Priority prio)
{
  if (milliseconds % 1000 == 0)
    source_ = g_timeout_source_new_seconds(milliseconds/1000);
  else
    source_ = g_timeout_source_new(milliseconds);

  g_source_set_priority(source_, prio);
  g_source_set_callback(source_, Callback, this, DestroyCallback);
}

void Timeout::Run(SourceCallback callback)
{
  if (!source_ || source_id_ || IsRunning())
    return;

  callback_ = callback;
  source_id_ = g_source_attach(source_, nullptr);
}

Idle::Idle(SourceCallback cb, Priority prio)
  : Source()
{
  Init(prio);
  Run(cb);
}

Idle::Idle(Priority prio)
  : Source()
{
  Init(prio);
}

void Idle::Init(Priority prio)
{
  source_ = g_idle_source_new();
  g_source_set_priority(source_, prio);
  g_source_set_callback(source_, Callback, this, DestroyCallback);
}

void Idle::Run(SourceCallback callback)
{
  if (!source_ || source_id_ || IsRunning())
    return;

  callback_ = callback;
  source_id_ = g_source_attach(source_, nullptr);
}

}
}

#endif
