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

#include "GLibSource.h"

namespace unity
{
namespace glib
{

Source::Source()
  : source_(nullptr)
  , source_id_(0)
{}

Source::~Source()
{
  Remove();
}

void Source::Remove()
{
  if (source_)
  {
    if (!g_source_is_destroyed(source_))
    {
      g_source_destroy(source_);
    }
  }

  if (source_)
  {
    g_source_unref(source_);
    source_ = nullptr;
  }
}

void Source::SetPriority(Priority prio)
{
  if (!source_)
    return;

  g_source_set_priority(source_, prio);
}

Source::Priority Source::GetPriority() const
{
  int prio = 0;
  if (source_)
    prio = g_source_get_priority(source_);

  return static_cast<Source::Priority>(prio);
}

bool Source::Run(SourceCallback callback)
{
  if (!source_ || source_id_ || IsRunning())
    return false;

  callback_ = callback;

  g_source_set_callback(source_, Callback, this, DestroyCallback);
  source_id_ = g_source_attach(source_, nullptr);

  return true;
}

bool Source::IsRunning() const
{
  if (!source_)
    return false;

  return (!g_source_is_destroyed(source_) && g_source_get_context(source_));
}

unsigned int Source::Id() const
{
  return source_id_;
}

gboolean Source::Callback(gpointer data)
{
  if (!data)
    return G_SOURCE_REMOVE;

  auto self = static_cast<Source*>(data);

  if (!self->callback_.empty() && self->callback_())
  {
    return G_SOURCE_CONTINUE;
  }
  else
  {
    return G_SOURCE_REMOVE;
  }
}

void Source::EmitRemovedSignal()
{
  if (source_id_)
  {
    removed.emit(Id());
  }
}

void Source::DestroyCallback(gpointer data)
{
  if (!data)
    return;

  static_cast<Source*>(data)->EmitRemovedSignal();
}


SourceManager::SourceManager()
{}

SourceManager::~SourceManager()
{
  for (auto it = sources_.begin(); it != sources_.end(); ++it)
  {
    auto source = *it;
    source->removed.clear();
    source->Remove();
  }
}

void SourceManager::Add(Source* source)
{
  Source::Ptr s(source);
  Add(s);
}

void SourceManager::Add(Source::Ptr const& source)
{
  if (!source)
    return;

  sources_.push_back(source);
  source->removed.connect(sigc::mem_fun(this, &SourceManager::OnSourceRemoved));
}

void SourceManager::OnSourceRemoved(unsigned int id)
{
  for (auto it = sources_.begin(); it != sources_.end(); ++it)
  {
    auto source = *it;

    if (source->Id() == id)
    {
      source->Remove();
      sources_.erase(it);
      break;
    }
  }
}

void SourceManager::Remove(unsigned int id)
{
  for (auto it = sources_.begin(); it != sources_.end(); ++it)
  {
    auto source = *it;

    if (source->Id() == id)
    {
      source->removed.clear();
      source->Remove();
      sources_.erase(it);
      break;
    }
  }
}

Source::Ptr SourceManager::GetSource(unsigned int id) const
{
  for (auto source : sources_)
  {
    if (source->Id() == id)
    {
      return source;
    }
  }

  return Source::Ptr();
}

}
}
