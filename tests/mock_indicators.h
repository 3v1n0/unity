/*
 * Copyright 2014 Canonical Ltd.
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 *
 */

#include <gmock/gmock.h>

namespace unity
{
namespace indicator
{
namespace
{

struct MockIndicators : Indicators
{
  typedef std::shared_ptr<MockIndicators> Ptr;
  typedef testing::NiceMock<MockIndicators> Nice;

  // Implementing Indicators virtual functions
  MOCK_METHOD2(SyncGeometries, void(std::string const&, EntryLocationMap const&));
  MOCK_METHOD5(ShowEntriesDropdown, void(Indicator::Entries const&, Entry::Ptr const&, unsigned xid, int x, int y));
  MOCK_METHOD2(OnEntryScroll, void(std::string const&, int delta));
  MOCK_METHOD5(OnEntryShowMenu, void(std::string const&, unsigned xid, int x, int y, unsigned button));
  MOCK_METHOD1(OnEntrySecondaryActivate, void(std::string const&));
  MOCK_METHOD3(OnShowAppMenu, void(unsigned xid, int x, int y));

  // Redirecting protected methods
  using Indicators::GetIndicator;
  using Indicators::ActivateEntry;
  using Indicators::AddIndicator;
  using Indicators::RemoveIndicator;
  using Indicators::SetEntryShowNow;
};

} // anonymous namespace
} // indicator namespace
} // unity namespace
