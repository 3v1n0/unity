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
 * Authored by: Nick Dedekind <nick.dedekinc@canonical.com>
 */

#include <gmock/gmock.h>
#include <UnityCore/PreviewPlayer.h>
#include <UnityCore/ConnectionManager.h>
#include "test_utils.h"
#include "config.h"

namespace unity
{

namespace
{
  const gchar* WHITE_NOISE = "file://" BUILDDIR "/tests/data/unity/sounds/whitenoise.mp3";

  void PlayAndWait(PreviewPlayer* player, std::string const& uri)
  {
    bool play_returned = false;
    auto play_callback = [&play_returned] (glib::Error const& error) {
      play_returned = true;
      EXPECT_TRUE(!error) << "Error: " << error.Message();
    };

    bool updated_called = false;
    auto updated_callback = [uri, &updated_called] (std::string const& _uri, PlayerState state, double) {
      updated_called = true;
      EXPECT_EQ(_uri, uri) << "Uri for PLAY not correct (" << _uri << " != " << _uri << ")";
      EXPECT_EQ((int)state, (int)PlayerState::PLAYING) << "Incorrect state returned on PLAY.";
    };

    connection::Wrapper conn(player->updated.connect(updated_callback));
    player->Play(uri, play_callback);
    ::Utils::WaitUntilMSec(play_returned, 3000, []() { return g_strdup("PLAY did not return"); });
    ::Utils::WaitUntilMSec(updated_called, 5000, []() { return g_strdup("Update not called on PLAY"); });
  }

  void PauseAndWait(PreviewPlayer* player)
  {
    bool pause_returned = false;
    auto callback = [&pause_returned] (glib::Error const& error) {
      pause_returned = true;
      EXPECT_TRUE(!error) << "Error: " << error.Message();
    };

    bool updated_called = false;
    auto updated_callback = [&updated_called] (std::string const&, PlayerState state, double) {
      updated_called = true;
      EXPECT_EQ((int)state, (int)PlayerState::PAUSED) << "Incorrect state returned on PAUSE.";
    };

    connection::Wrapper conn(player->updated.connect(updated_callback));
    player->Pause(callback);
    ::Utils::WaitUntilMSec(pause_returned, 3000, []() { return g_strdup("PAUSE did not return"); });
    ::Utils::WaitUntilMSec(updated_called, 5000, []() { return g_strdup("Update not called om PAUSE"); });
  }

  void ResumeAndWait(PreviewPlayer* player)
  {
    bool resume_returned = false;
    auto callback = [&resume_returned] (glib::Error const& error) {
      resume_returned = true;
      EXPECT_TRUE(!error) << "Error: " << error.Message();
    };

    bool updated_called = false;
    auto updated_callback = [&updated_called] (std::string const&, PlayerState state, double) {
      updated_called = true;
      EXPECT_EQ((int)state, (int)PlayerState::PLAYING) << "Incorrect state returned on RESUME.";
    };

    connection::Wrapper conn(player->updated.connect(updated_callback));
    player->Resume(callback);
    ::Utils::WaitUntilMSec(resume_returned, 3000, []() { return g_strdup("RESUME did not return"); });
    ::Utils::WaitUntilMSec(updated_called, 5000, []() { return g_strdup("Update not called on RESUME"); });
  }

  void StopAndWait(PreviewPlayer* player)
  {
    bool stop_returned = false;
    auto callback = [&stop_returned] (glib::Error const& error) {
      stop_returned = true;
      EXPECT_TRUE(!error) << "Error: " << error.Message();
    };

    bool updated_called = false;
    auto updated_callback = [&updated_called] (std::string const&, PlayerState state, double) {
      updated_called = true;
      EXPECT_EQ((int)state, (int)PlayerState::STOPPED) << "Incorrect state returned on STOP.";
    };

    connection::Wrapper conn(player->updated.connect(updated_callback));
    player->Stop(callback);
    ::Utils::WaitUntilMSec(stop_returned, 3000, []() { return g_strdup("STOP did not return"); });
    ::Utils::WaitUntilMSec(updated_called, 5000, []() { return g_strdup("Update not called on STOP"); });
  }
}

struct TestPreviewPlayer : testing::Test
{
  PreviewPlayer player;
};

TEST_F(TestPreviewPlayer, TestConstruct)
{
  PreviewPlayer player1;
}

TEST_F(TestPreviewPlayer, TestPlayerControl)
{
  PlayAndWait(&player, WHITE_NOISE);

  PauseAndWait(&player);

  ResumeAndWait(&player);

  StopAndWait(&player);
}

TEST_F(TestPreviewPlayer, TestMultiPlayer)
{
  {
    PreviewPlayer player2;
    PlayAndWait(&player2, WHITE_NOISE);
  }

  StopAndWait(&player);
}


} // namespace unity