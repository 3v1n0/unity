/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program. If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include <glib.h>
#include <gmock/gmock.h>

#include "BamfApplicationManager.h"
#include "bamf-mock-application.h"
#include "bamf-mock-window.h"
#include "mock-application.h"
#include "StandaloneWindowManager.h"

#include <UnityCore/GLibWrapper.h>

namespace {

unity::StandaloneWindow::Ptr AddFakeWindowToWM(Window xid, bool mapped)
{
  auto fake_window = std::make_shared<unity::StandaloneWindow>(xid);
  fake_window->mapped = mapped;

  unity::StandaloneWindowManager* wm = dynamic_cast<unity::StandaloneWindowManager*>(&unity::WindowManager::Default());
  wm->AddStandaloneWindow(fake_window);

  return fake_window;
}

struct TestBamfApplication : public testing::Test
{
  TestBamfApplication()
    : bamf_mock_application_(bamf_mock_application_new())
    , application_(mock_manager_, unity::glib::object_cast<BamfApplication>(bamf_mock_application_))
  {}

  testmocks::MockApplicationManager::Nice mock_manager_;
  unity::glib::Object<BamfMockApplication> bamf_mock_application_;
  unity::bamf::Application application_;
};

TEST_F(TestBamfApplication, GetWindows)
{
  GList* children = nullptr;
  for (int i = 0; i<5; ++i)
  {
  	BamfMockWindow* window = bamf_mock_window_new();
  	bamf_mock_window_set_xid(window, i);
    children = g_list_append(children, window);
  }
  bamf_mock_application_set_children(bamf_mock_application_, children);

  AddFakeWindowToWM(0, true);
  AddFakeWindowToWM(1, true);
  AddFakeWindowToWM(2, false);
  AddFakeWindowToWM(3, true);
  AddFakeWindowToWM(4, false);

  auto windows = application_.GetWindows();
  ASSERT_EQ(windows.size(), 5);

  g_list_free_full(children, g_object_unref);
}

}