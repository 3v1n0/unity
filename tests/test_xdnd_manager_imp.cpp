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
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#include <gmock/gmock.h>
using namespace testing;

#include "launcher/XdndManagerImp.h"
#include "launcher/XdndCollectionWindow.h"
#include "launcher/XdndStartStopNotifier.h"

#include "test_utils.h"

#include <Nux/Nux.h>
#include <X11/Xlib.h>

namespace {

class MockXdndStartStopNotifier : public unity::XdndStartStopNotifier {
public:
  typedef std::shared_ptr<MockXdndStartStopNotifier> Ptr;
};

class MockXdndCollectionWindow : public unity::XdndCollectionWindow {
public:
  typedef std::shared_ptr<MockXdndCollectionWindow> Ptr;

  MOCK_METHOD0(Collect, void(void));
  MOCK_METHOD0(Deactivate, void(void));
  MOCK_METHOD1(GetData, std::string(std::string const& type));
};

class TestXdndManager : public Test {
public:
  TestXdndManager()
    : xdnd_start_stop_notifier_(new MockXdndStartStopNotifier())
    , xdnd_collection_window_(new MockXdndCollectionWindow())
    , xdnd_manager(xdnd_start_stop_notifier_, xdnd_collection_window_)
  {}

  MockXdndStartStopNotifier::Ptr xdnd_start_stop_notifier_;
  MockXdndCollectionWindow::Ptr xdnd_collection_window_;
  unity::XdndManagerImp xdnd_manager;
};

TEST_F(TestXdndManager, SignalDndStartedAndFinished)
{
  std::vector<std::string> mimes = {"text/uri-list", "hello/world"};

  auto emit_collected_signal = [&] () {
    xdnd_collection_window_->collected.emit(mimes);
  };

  EXPECT_CALL(*xdnd_collection_window_, Collect())
    .WillOnce(Invoke(emit_collected_signal));

  EXPECT_CALL(*xdnd_collection_window_, GetData("text/uri-list"))
    .WillOnce(Return("file://dnd_file"));

  bool dnd_started_emitted = false;
  xdnd_manager.dnd_started.connect([&] (std::string const& /*data*/, int /*monitor*/) {
  	dnd_started_emitted = true;
  });

  xdnd_start_stop_notifier_->started.emit();
  ASSERT_TRUE(dnd_started_emitted);

  EXPECT_CALL(*xdnd_collection_window_, Deactivate())
    .Times(1);

  bool dnd_finished_emitted = false;
  xdnd_manager.dnd_finished.connect([&] () {
    dnd_finished_emitted = true;
  });

  xdnd_start_stop_notifier_->finished.emit();
  EXPECT_TRUE(dnd_finished_emitted);
}

TEST_F(TestXdndManager, SignalDndStarted_InvalidMimes)
{
  std::vector<std::string> mimes = {"invalid/mimes", "goodbye/world"};

  auto emit_collected_signal = [&] () {
    xdnd_collection_window_->collected.emit(mimes);
  };

  EXPECT_CALL(*xdnd_collection_window_, Collect())
    .Times(1)
    .WillOnce(Invoke(emit_collected_signal));

  EXPECT_CALL(*xdnd_collection_window_, GetData(_)).Times(0);

  bool dnd_started_emitted = false;
  xdnd_manager.dnd_started.connect([&] (std::string const& /*data*/, int /*monitor*/) {
    dnd_started_emitted = true;
  });

  xdnd_start_stop_notifier_->started.emit();

  EXPECT_FALSE(dnd_started_emitted);
}

TEST_F(TestXdndManager, SignalDndStarted_InvalidData)
{
  std::vector<std::string> mimes = {"text/uri-list", "hello/world"};

  auto emit_collected_signal = [&] () {
    xdnd_collection_window_->collected.emit(mimes);
  };

  EXPECT_CALL(*xdnd_collection_window_, Collect())
    .WillOnce(Invoke(emit_collected_signal));

  EXPECT_CALL(*xdnd_collection_window_, GetData("text/uri-list"))
    .WillOnce(Return(""));

  bool dnd_started_emitted = false;
  xdnd_manager.dnd_started.connect([&] (std::string const& /*data*/, int /*monitor*/) {
    dnd_started_emitted = true;
  });

  xdnd_start_stop_notifier_->started.emit();
  EXPECT_FALSE(dnd_started_emitted);
}

}
