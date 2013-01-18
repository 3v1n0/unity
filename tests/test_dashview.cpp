/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#include <list>

#include <gtest/gtest.h>

#include <Nux/Nux.h>
#include <NuxCore/ObjectPtr.h>

#include "dash/DashView.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UnitySettings.h"

using namespace unity;
using namespace unity::dash;

namespace
{

class TestDashView : public ::testing::Test
{
public:
  TestDashView() {}

  Settings unity_settings_;
  dash::Style dash_style_;
  panel::Style panel_style_;
};

TEST_F(TestDashView, TestSetQueries)
{
  nux::ObjectPtr<DashView> view(new dash::DashView());
}

}
