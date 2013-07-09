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
#include <UnityCore/GLibDBusServer.h>
#include <UnityCore/Variant.h>
#include "test_utils.h"
#include "config.h"

namespace unity
{

namespace
{
  const std::string WHITE_NOISE = "file://" BUILDDIR "/tests/data/unity/sounds/whitenoise.mp3";

  const std::string PLAYER_NAME = "com.canonical.Unity.Lens.Music.PreviewPlayer";
  const std::string PLAYER_PATH = "/com/canonical/Unity/Lens/Music/PreviewPlayer";
  const std::string PLAYER_INTERFACE =
  R"(<node>
    <interface name="com.canonical.Unity.Lens.Music.PreviewPlayer">
      <method name="Play">
        <arg type="s" name="uri" direction="in"/>
      </method>
      <method name="Pause">
      </method>
      <method name="Resume">
      </method>
      <method name="PauseResume">
      </method>
      <method name="Stop">
      </method>
      <method name="Close">
      </method>
      <method name="VideoProperties">
        <arg type="s" name="uri" direction="in"/>
        <arg type="a{sv}" name="result" direction="out"/>
      </method>
      <signal name="Progress">
        <arg type="s" name="uri"/>
        <arg type="u" name="state"/>
        <arg type="d" name="value"/>
      </signal>
    </interface>
  </node>)";

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
    ::Utils::WaitUntilMSec(updated_called, 5000, []() { return g_strdup("Update not called on PAUSE"); });
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

  struct FakeRemotePlayer
  {
    typedef std::shared_ptr<FakeRemotePlayer> Ptr;
    FakeRemotePlayer()
      : server_(PLAYER_NAME)
    {
      server_.AddObjects(PLAYER_INTERFACE, PLAYER_PATH);
      auto const& object = server_.GetObjects().front();

      object->SetMethodsCallsHandler([this, object] (std::string const& method, GVariant* parameters) {
        if (method == "Play")
        {
          current_uri_ = glib::Variant(parameters).GetString();
          object->EmitSignal("Progress", g_variant_new("(sud)", current_uri_.c_str(), PlayerState::PLAYING, 0));
        }
        else if (method == "Pause")
        {
          object->EmitSignal("Progress", g_variant_new("(sud)", current_uri_.c_str(), PlayerState::PAUSED, 0));
        }
        else if (method == "Resume")
        {
          object->EmitSignal("Progress", g_variant_new("(sud)", current_uri_.c_str(), PlayerState::PLAYING, 0));
        }
        else if (method == "Stop")
        {
          object->EmitSignal("Progress", g_variant_new("(sud)", current_uri_.c_str(), PlayerState::STOPPED, 0));
          current_uri_ = "";
        }

        return static_cast<GVariant*>(nullptr);
      });
    }

  private:
    glib::DBusServer server_;
    std::string current_uri_;
  };
}

struct TestPreviewPlayer : testing::Test
{
  static void SetUpTestCase()
  {
    remote_player_ = std::make_shared<FakeRemotePlayer>();
  }

  static void TearDownTestCase()
  {
    remote_player_.reset();
  }

  static FakeRemotePlayer::Ptr remote_player_;
  PreviewPlayer player;
};

FakeRemotePlayer::Ptr TestPreviewPlayer::remote_player_;

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