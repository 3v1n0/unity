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
#include "UnityCore/PreviewPlayer.h"
#include "test_utils.h"

namespace unity
{

TEST(TestPreviewPlayer, TestConstruct)
{
  PreviewPlayer player1;
  PreviewPlayer player2;
}

TEST(TestPreviewPlayer, TestPlay)
{
  PreviewPlayer player;

  bool play_returned = false;
  auto play_callback = [&play_returned] (glib::Error const& error) {
    play_returned = true;
    EXPECT_TRUE(!error) << "Error: " << error.Message();
  };
  
  bool updated_called = false;
  auto updated_callback = [&updated_called] (std::string const&, PlayerState, double) {
    updated_called = true;
  };

  player.updated.connect(updated_callback);
  player.Play("https://3rdpartymedia.ubuntuone.com/7digital/previews/clips/34/1218209.clip.mp3", play_callback);
  ::Utils::WaitUntilMSec(play_returned, 3000);
  ::Utils::WaitUntilMSec(updated_called, 5000);
}

TEST(TestPreviewPlayer, TestPause)
{
  PreviewPlayer player;

  bool pause_returned = false;
  auto callback = [&pause_returned] (glib::Error const& error) {
    pause_returned = true;
    EXPECT_TRUE(!error) << "Error: " << error.Message();
  };

  player.Pause(callback);
  ::Utils::WaitUntilMSec(pause_returned, 3000);
}

TEST(TestPreviewPlayer, TestResume)
{
  PreviewPlayer player;

  bool resume_returned = false;
  auto callback = [&resume_returned] (glib::Error const& error) {
    resume_returned = true;
    EXPECT_TRUE(!error) << "Error: " << error.Message();
  };

  player.Resume(callback);
  ::Utils::WaitUntilMSec(resume_returned, 3000);
}

TEST(TestPreviewPlayer, TestStop)
{
  PreviewPlayer player;

  bool stop_returned = false;
  auto callback = [&stop_returned] (glib::Error const& error) {
    stop_returned = true;
    EXPECT_TRUE(!error) << "Error: " << error.Message();
  };

  player.Stop(callback);
  ::Utils::WaitUntilMSec(stop_returned, 3000);
}

TEST(TestPreviewPlayer, TestMultiPlayer)
{
  PreviewPlayer player1;
  {
    PreviewPlayer player2;

    bool play_returned = false;
    auto callback = [&play_returned] (glib::Error const& error) {
      play_returned = true;
      EXPECT_TRUE(!error) << "Error: " << error.Message();
    };

    player2.Play("https://3rdpartymedia.ubuntuone.com/7digital/previews/clips/34/1218209.clip.mp3", callback);
    ::Utils::WaitUntilMSec(play_returned, 3000);
  }

  bool stop_returned = false;
  auto callback = [&stop_returned] (glib::Error const& error) {
    stop_returned = true;
    EXPECT_TRUE(!error) << "Error: " << error.Message();
  };

  player1.Stop(callback);
  ::Utils::WaitUntilMSec(stop_returned, 3000);
}


} // namespace unity