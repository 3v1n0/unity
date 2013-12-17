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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>
#include <glib.h>
#include "DecorationsWidgets.h"

namespace
{
using namespace unity::decoration;
using namespace testing;

struct MockItem : public SimpleItem
{
  typedef NiceMock<MockItem> Nice;
  typedef std::shared_ptr<MockItem> Ptr;

  MockItem()
  {
    visible = true;
    ON_CALL(*this, GetNaturalWidth()).WillByDefault(Invoke([this] { return SimpleItem::GetNaturalWidth(); }));
    ON_CALL(*this, GetNaturalHeight()).WillByDefault(Invoke([this] { return SimpleItem::GetNaturalHeight(); }));
    ON_CALL(*this, SetCoords(_, _)).WillByDefault(Invoke([this] (int x, int y) { SimpleItem::SetCoords(x, y); }));
    ON_CALL(*this, SetX(_)).WillByDefault(Invoke([this] (int x) { SimpleItem::SetX(x); }));
    ON_CALL(*this, SetY(_)).WillByDefault(Invoke([this] (int y) { SimpleItem::SetY(y); }));
    ON_CALL(*this, SetSize(_, _)).WillByDefault(Invoke([this] (int w, int h) { SimpleItem::SetSize(w, h); }));
    ON_CALL(*this, SetWidth(_)).WillByDefault(Invoke([this] (int w) { SimpleItem::SetWidth(w); }));
    ON_CALL(*this, SetHeight(_)).WillByDefault(Invoke([this] (int h) { SimpleItem::SetHeight(h); }));
    ON_CALL(*this, SetMaxWidth(_)).WillByDefault(Invoke([this] (int mw) { SimpleItem::SetMaxWidth(mw); }));
    ON_CALL(*this, SetMaxHeight(_)).WillByDefault(Invoke([this] (int mh) { SimpleItem::SetMaxHeight(mh); }));
    ON_CALL(*this, SetMinWidth(_)).WillByDefault(Invoke([this] (int mw) { SimpleItem::SetMinWidth(mw); }));
    ON_CALL(*this, SetMinHeight(_)).WillByDefault(Invoke([this] (int mh) { SimpleItem::SetMinHeight(mh); }));
    ON_CALL(*this, MotionEvent(_)).WillByDefault(Invoke([this] (CompPoint const& p) { SimpleItem::MotionEvent(p); }));
    ON_CALL(*this, ButtonDownEvent(_, _)).WillByDefault(Invoke([this] (CompPoint const& p, unsigned b) { SimpleItem::ButtonDownEvent(p, b); }));
    ON_CALL(*this, ButtonUpEvent(_, _)).WillByDefault(Invoke([this] (CompPoint const& p, unsigned b) { SimpleItem::ButtonUpEvent(p, b); }));
  }

  MOCK_CONST_METHOD0(GetNaturalWidth, int());
  MOCK_CONST_METHOD0(GetNaturalHeight, int());
  MOCK_METHOD2(SetCoords, void(int, int));
  MOCK_METHOD1(SetX, void(int));
  MOCK_METHOD1(SetY, void(int));
  MOCK_METHOD2(SetSize, void(int, int));
  MOCK_METHOD1(SetWidth, void(int));
  MOCK_METHOD1(SetHeight, void(int));
  MOCK_METHOD1(SetMaxWidth, void(int));
  MOCK_METHOD1(SetMaxHeight, void(int));
  MOCK_METHOD1(SetMinWidth, void(int));
  MOCK_METHOD1(SetMinHeight, void(int));

  MOCK_METHOD1(MotionEvent, void(CompPoint const&));
  MOCK_METHOD2(ButtonDownEvent, void(CompPoint const&, unsigned));
  MOCK_METHOD2(ButtonUpEvent, void(CompPoint const&, unsigned));

  using SimpleItem::geo_parameters_changed;
  using SimpleItem::rect_;
  using SimpleItem::natural_;
  using SimpleItem::max_;
  using SimpleItem::min_;
};

MockItem::Ptr SizedMockItem(int w, int h)
{
  auto item = std::make_shared<MockItem::Nice>();
  item->SetSize(w, h);
  return item;
}

MockItem::Ptr RandomMockItem()
{
  return SizedMockItem(g_random_int_range(10, 100), g_random_int_range(10, 100));
}

}
