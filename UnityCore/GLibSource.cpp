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
  , callback_data_(nullptr)
{}

Source::~Source()
{
  Remove();

  if (callback_data_)
    callback_data_->self = nullptr;
}

void Source::Remove()
{
  if (source_)
  {
    if (!g_source_is_destroyed(source_))
    {
      g_source_destroy(source_);
    }

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

  return static_cast<Priority>(prio);
}

bool Source::Run(Callback const& callback)
{
  if (!source_ || source_id_ || IsRunning())
    return false;

  callback_data_ = new CallBackData(this, callback);

  g_source_set_callback(source_, SourceCallback, callback_data_, DestroyCallback);
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

gboolean Source::SourceCallback(gpointer data)
{
  if (!data)
    return G_SOURCE_REMOVE;

  auto cb_data = static_cast<CallBackData*>(data);

  if (cb_data && cb_data->callback_fn_ && cb_data->callback_fn_())
  {
    return G_SOURCE_CONTINUE;
  }

  return G_SOURCE_REMOVE;
}

void Source::DestroyCallback(gpointer data)
{
  if (!data)
    return;

  auto cb_data = static_cast<CallBackData*>(data);

  if (cb_data->self)
  {
    auto self = cb_data->self;
    self->callback_data_ = nullptr;

    if (self->Id())
      self->removed.emit(self->Id());
  }

  delete cb_data;
}


Timeout::Timeout(unsigned int milliseconds, Callback const& cb, Priority prio)
{
  Init(milliseconds, prio);
  Run(cb);
}

Timeout::Timeout(unsigned int milliseconds, Priority prio)
{
  Init(milliseconds, prio);
}

void Timeout::Init(unsigned int milliseconds, Priority prio)
{
  source_ = g_timeout_source_new(milliseconds);
  SetPriority(prio);
}


TimeoutSeconds::TimeoutSeconds(unsigned int seconds, Callback const& cb, Priority prio)
{
  Init(seconds, prio);
  Run(cb);
}

TimeoutSeconds::TimeoutSeconds(unsigned int seconds, Priority prio)
{
  Init(seconds, prio);
}

void TimeoutSeconds::Init(unsigned int seconds, Priority prio)
{
  source_ = g_timeout_source_new_seconds(seconds);
  SetPriority(prio);
}


Idle::Idle(Callback const& cb, Priority prio)
{
  Init(prio);
  Run(cb);
}

Idle::Idle(Priority prio)
{
  Init(prio);
}

void Idle::Init(Priority prio)
{
  source_ = g_idle_source_new();
  SetPriority(prio);
}


SourceManager::SourceManager()
{}

SourceManager::~SourceManager()
{
  for (auto it = sources_.begin(); it != sources_.end();)
  {
    RemoveItem(it++);
  }
}

bool SourceManager::Add(Source* source, std::string const& nick)
{
  Source::Ptr s(source);
  return Add(s, nick);
}

bool SourceManager::Add(Source::Ptr const& source, std::string const& nick)
{
  if (!source)
    return false;

  /* If the manager controls another source equal to the one we're passing,
   * we don't add it again */
  for (auto it : sources_)
  {
    if (it.second == source)
    {
      return false;
    }
  }

  std::string source_nick(nick);

  if (source_nick.empty())
  {
    /* If we don't have a nick, we use the source pointer string as nick. */
    source_nick = std::to_string(reinterpret_cast<uintptr_t>(source.get()));
  }

  auto old_source_it = sources_.find(source_nick);
  if (old_source_it != sources_.end())
  {
    /* If a source with the same nick has been found, we can safely replace it,
     * since at this point we're sure that they refers to two different sources.
     * This should not happen when we're trying to re-add the same source from
     * its callback. */
    RemoveItem(old_source_it);
  }

  sources_[source_nick] = source;
  source->removed.connect(sigc::mem_fun(this, &SourceManager::OnSourceRemoved));

  return true;
}

Source::Ptr SourceManager::AddTimeout(unsigned int milliseconds, std::string const& nick)
{
  auto timeout = std::make_shared<Timeout>(milliseconds);

  if (Add(timeout, nick))
  {
    return timeout;
  }

  return nullptr;
}

Source::Ptr SourceManager::AddTimeout(unsigned int milliseconds, Source::Callback const& cb, std::string const& nick)
{
  auto timeout = std::make_shared<Timeout>(milliseconds);

  if (Add(timeout, nick))
  {
    timeout->Run(cb);
    return timeout;
  }

  return nullptr;
}

Source::Ptr SourceManager::AddTimeoutSeconds(unsigned int seconds, std::string const& nick)
{
  auto timeout = std::make_shared<Timeout>(seconds);

  if (Add(timeout, nick))
  {
    return timeout;
  }

  return nullptr;
}

Source::Ptr SourceManager::AddTimeoutSeconds(unsigned int seconds, Source::Callback const& cb, std::string const& nick)
{
  auto timeout = std::make_shared<TimeoutSeconds>(seconds);

  if (Add(timeout, nick))
  {
    timeout->Run(cb);
    return timeout;
  }

  return nullptr;
}

Source::Ptr SourceManager::AddIdle(std::string const& nick)
{
  auto idle = std::make_shared<Idle>();

  if (Add(idle, nick))
  {
    return idle;
  }

  return nullptr;
}

Source::Ptr SourceManager::AddIdle(Source::Callback const& cb, std::string const& nick)
{
  auto idle = std::make_shared<Idle>();

  if (Add(idle, nick))
  {
    idle->Run(cb);
    return idle;
  }

  return nullptr;
}

void SourceManager::OnSourceRemoved(unsigned int id)
{
  for (auto it = sources_.begin(); it != sources_.end(); ++it)
  {
    auto source = it->second;

    if (source->Id() == id)
    {
      sources_.erase(it);
      break;
    }
  }
}

bool SourceManager::Remove(std::string const& nick)
{
  auto it = sources_.find(nick);

  if (it != sources_.end())
  {
    RemoveItem(it);
    return true;
  }

  return false;
}

bool SourceManager::Remove(unsigned int id)
{
  for (auto it = sources_.begin(); it != sources_.end(); ++it)
  {
    auto source = it->second;

    if (source->Id() == id)
    {
      RemoveItem(it);
      return true;
    }
  }

  return false;
}

void SourceManager::RemoveItem(SourcesMap::iterator it)
{
  auto source = it->second;

  source->removed.clear();
  source->Remove();
  sources_.erase(it);
}

Source::Ptr SourceManager::GetSource(unsigned int id) const
{
  for (auto it : sources_)
  {
    if (it.second->Id() == id)
    {
      return it.second;
    }
  }

  return Source::Ptr();
}

Source::Ptr SourceManager::GetSource(std::string const& nick) const
{
  auto it = sources_.find(nick);

  if (it != sources_.end())
  {
    return it->second;
  }

  return nullptr;
}

}
}
