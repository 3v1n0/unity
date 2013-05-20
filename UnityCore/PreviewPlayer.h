// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */


#ifndef PREVIEW_PLAYER_H
#define PREVIEW_PLAYER_H

#include "GLibWrapper.h"
#include "GLibSignal.h"
#include <memory>
#include <sigc++/signal.h>

namespace unity
{

enum class PlayerState
{
  STOPPED,
  PLAYING,
  PAUSED
};


class PlayerInterface
{
public:
  typedef std::shared_ptr<PlayerInterface> Ptr;
  virtual ~PlayerInterface() {}

  typedef std::function<void(glib::Error const&)> Callback;

  virtual void Play(std::string const& uri, Callback const& callback = nullptr) = 0;
  virtual void Pause(Callback const& callback = nullptr) = 0;
  virtual void Resume(Callback const& callback = nullptr) = 0;
  virtual void Stop(Callback const& callback = nullptr) = 0;

  sigc::signal<void, std::string const&, PlayerState, double> updated;
};

class PreviewPlayerImpl;

class PreviewPlayer : public PlayerInterface
{
public:
  PreviewPlayer();
  ~PreviewPlayer();

  virtual void Play(std::string const& uri, Callback const& callback = nullptr);
  virtual void Pause(Callback const& callback = nullptr);
  virtual void Resume(Callback const& callback = nullptr);
  virtual void Stop(Callback const& callback = nullptr);

private:
  std::weak_ptr<PreviewPlayerImpl> pimpl;
  glib::Signal<void, gpointer, const gchar*, guint32, double> progress_signal_;
};

} // namespace unity

#endif // PREVIEW_PLAYER_H