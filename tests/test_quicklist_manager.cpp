/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include <gtest/gtest.h>

#include <Nux/Nux.h>
#include <NuxCore/ObjectPtr.h>

#include "launcher/QuicklistManager.h"
#include "launcher/QuicklistView.h"
#include "unity-shared/UnitySettings.h"

namespace {

char buf[sizeof(unity::QuicklistView) * 3];

struct MockQuicklistView : public unity::QuicklistView
{
  void *operator new(size_t uiSize) {
    GObjectStats._allocation_list.push_back(buf);
    return (void *) buf;
  }

  void  operator delete(void *p) {
    // Dont' remove me!
  }
};

TEST(TestQuicklistManager, RegisterQuicklist)
{
  unity::Settings unity_settings;
  nux::ObjectWeakPtr<unity::QuicklistView> ptr;

  {
    nux::ObjectPtr<unity::QuicklistView> quicklist1(new MockQuicklistView);
    ptr = quicklist1;
    ASSERT_EQ(quicklist1->GetReferenceCount(), 1);
    ASSERT_TRUE(unity::QuicklistManager::Default()->RegisterQuicklist(quicklist1));
    ASSERT_EQ(quicklist1->GetReferenceCount(), 1);
    ASSERT_FALSE(unity::QuicklistManager::Default()->RegisterQuicklist(quicklist1));
    ASSERT_EQ(quicklist1->GetReferenceCount(), 1);
  }
  
  ASSERT_FALSE(ptr.IsValid());

  nux::ObjectPtr<unity::QuicklistView> quicklist2(new MockQuicklistView);
  ASSERT_EQ(quicklist2->GetReferenceCount(), 1);
  ASSERT_TRUE(unity::QuicklistManager::Default()->RegisterQuicklist(quicklist2));
  ASSERT_EQ(quicklist2->GetReferenceCount(), 1);
}

}
