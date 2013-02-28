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

#include "PreviewPlayer.h"
#include <unity-protocol.h>
#include <NuxCore/NuxCore.h>
#include <NuxCore/Logger.h>


namespace unity
{

DECLARE_LOGGER(logger, "unity.previewplayer");

std::shared_ptr<PreviewPlayerImpl> player_impl;

class PreviewPlayerImpl
{
public:
  PreviewPlayerImpl()
  : player_(unity_protocol_preview_player_new())
  {
  }

  ~PreviewPlayerImpl()
  {
    unity_protocol_preview_player_close(player_, OnAsyncCallback, new AsyncCallData(unity_protocol_preview_player_close_finish, nullptr));
  }

  static void Init()
  {
    if (!player_impl)
      player_impl.reset(new PreviewPlayerImpl());
  }

  void Reference() { ref_count.Increment(); }

  bool Dereference()
  {
    if (ref_count.Decrement() == 0)
    {
      player_impl.reset();
      return false;
    }
    return true;
  }

  struct AsyncCallData
  {
    typedef void (*FinishFunc) (UnityProtocolPreviewPlayer* self, GAsyncResult* _res_, GError** error);

    AsyncCallData(FinishFunc _proto_func, PlayerInterface::Callback _callback):finish_func(_proto_func),callback(_callback) {}

    FinishFunc finish_func;
    PlayerInterface::Callback callback;
  };

  static void OnAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data)
  {
    std::unique_ptr<AsyncCallData> data((AsyncCallData*)user_data);
    glib::Error error;
    data->finish_func(UNITY_PROTOCOL_PREVIEW_PLAYER (source_object), res, &error);
    
    if (data->callback)
      data->callback(error);
  }

  void Play(std::string const& uri, PlayerInterface::Callback const& callback)
  {
    LOG_INFO(logger) << "Play '" << uri << "'";
    unity_protocol_preview_player_play(player_, uri.c_str(), OnAsyncCallback, new AsyncCallData(unity_protocol_preview_player_play_finish, callback));
  }

  void Pause(PlayerInterface::Callback const& callback)
  {
    LOG_INFO(logger) << "Pause";
    unity_protocol_preview_player_pause(player_, OnAsyncCallback, new AsyncCallData(unity_protocol_preview_player_pause_finish, callback));
  }

  void Resume(PlayerInterface::Callback const& callback)
  {
    LOG_INFO(logger) << "Resume";
    unity_protocol_preview_player_resume(player_, OnAsyncCallback, new AsyncCallData(unity_protocol_preview_player_resume_finish, callback));
  }

  void Stop(PlayerInterface::Callback const& callback)
  {
    LOG_INFO(logger) << "Stop";
    unity_protocol_preview_player_stop(player_, OnAsyncCallback, new AsyncCallData(unity_protocol_preview_player_stop_finish, callback));
  }

  glib::Object<UnityProtocolPreviewPlayer> player_;
  nux::NThreadSafeCounter ref_count;
};

PreviewPlayer::PreviewPlayer()
: pimpl((PreviewPlayerImpl::Init(), player_impl))
{
  if (auto const& p = pimpl.lock())
  {
    p->Reference();
    progress_signal_.Connect(p->player_, "progress", [this](gpointer, const gchar* uri, guint32 state, double progress) {
      updated.emit(glib::gchar_to_string(uri), PlayerState(state), progress);
    });
  }
  else
    LOG_ERROR(logger) << "Failed to aquire player";
}

PreviewPlayer::~PreviewPlayer()
{
  if (auto const& p = pimpl.lock())
    p->Dereference();
  else
    LOG_ERROR(logger) << "Failed to aquire player";
}

void PreviewPlayer::Play(std::string const& uri, Callback const& callback)
{
  if (auto const& p = pimpl.lock())
    p->Play(uri, callback);
  else
    LOG_ERROR(logger) << "Failed to aquire player";
}

void PreviewPlayer::Pause(Callback const& callback)
{
  if (auto const& p = pimpl.lock())
    p->Pause(callback);
  else
    LOG_ERROR(logger) << "Failed to aquire player";
}

void PreviewPlayer::Resume(Callback const& callback)
{
  if (auto const& p = pimpl.lock())
    p->Resume(callback);
  else
    LOG_ERROR(logger) << "Failed to aquire player";
}

void PreviewPlayer::Stop(Callback const& callback)
{
  if (auto const& p = pimpl.lock())
    p->Stop(callback);
  else
    LOG_ERROR(logger) << "Failed to aquire player";
}

} // namespace unity
