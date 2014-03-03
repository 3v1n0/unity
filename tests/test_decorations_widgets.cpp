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

#include "decoration_mock_item.h"

namespace
{
using namespace testing;

int random_positive_int()
{
  return g_random_int_range(0, std::numeric_limits<short>::max()/2);
}

int random_int()
{
  return g_random_int_range(std::numeric_limits<short>::min()/2, std::numeric_limits<short>::max()/2);
}

struct SigReceiver : sigc::trackable
{
  typedef NiceMock<SigReceiver> Nice;

  SigReceiver(MockItem const& item)
  {
    const_cast<MockItem&>(item).geo_parameters_changed.connect(sigc::mem_fun(this, &SigReceiver::GeoChanged));
  }

  MOCK_METHOD0(GeoChanged, void());
};

struct TestDecorationItem : Test
{
  TestDecorationItem()
   : sig_receiver(item)
  {
    item.SetSize(g_random_int_range(10, 100), g_random_int_range(10, 30));
  }

  MockItem::Nice item;
  SigReceiver::Nice sig_receiver;
};

struct MockBasicContainer : BasicContainer
{
  MOCK_METHOD0(DoRelayout, void());
};

TEST_F(TestDecorationItem, DefaultVisibilty)
{
  EXPECT_TRUE(item.visible());
}

TEST_F(TestDecorationItem, DefaultSensitivity)
{
  EXPECT_TRUE(item.sensitive());
}

TEST_F(TestDecorationItem, DefaultMouseOwnership)
{
  EXPECT_FALSE(item.mouse_owner());
}

TEST_F(TestDecorationItem, DefaultFocused)
{
  EXPECT_FALSE(item.focused());
}

TEST_F(TestDecorationItem, DefaultScale)
{
  EXPECT_FLOAT_EQ(1.0f, item.scale());
}

TEST_F(TestDecorationItem, DefaultMaxSize)
{
  MockItem item;
  EXPECT_EQ(item.max_, nux::Size(std::numeric_limits<short>::max(), std::numeric_limits<short>::max()));
}

TEST_F(TestDecorationItem, DefaultMinSize)
{
  MockItem item;
  EXPECT_EQ(item.min_, nux::Size(0, 0));
}

TEST_F(TestDecorationItem, DefaultNaturalSize)
{
  MockItem item;
  EXPECT_EQ(item.natural_, nux::Size(0, 0));
}

TEST_F(TestDecorationItem, DefaultParent)
{
  EXPECT_EQ(nullptr, item.GetParent());
}

TEST_F(TestDecorationItem, SetParent)
{
  auto parent = std::make_shared<MockBasicContainer>();
  item.SetParent(parent);
  EXPECT_EQ(parent, item.GetParent());
}

TEST_F(TestDecorationItem, WeakParent)
{
  auto parent = std::make_shared<MockBasicContainer>();
  item.SetParent(parent);
  parent.reset();
  EXPECT_EQ(nullptr, item.GetParent());
}

TEST_F(TestDecorationItem, DefaultTopParent)
{
  EXPECT_EQ(nullptr, item.GetTopParent());
}

TEST_F(TestDecorationItem, SimpleTopParent)
{
  auto parent = std::make_shared<MockBasicContainer>();
  item.SetParent(parent);
  EXPECT_EQ(parent, item.GetTopParent());
}

TEST_F(TestDecorationItem, TopParent)
{
  Item::List containers;
  auto parent = std::make_shared<MockBasicContainer>();
  auto top = parent;

  for (int i = 0; i < 10; ++i)
  {
    auto c = std::make_shared<MockBasicContainer>();
    containers.push_back(c);
    c->SetParent(parent);
    parent = c;
    ASSERT_EQ(top, c->GetTopParent());
  }

  item.SetParent(parent);
  ASSERT_EQ(parent, item.GetParent());
  EXPECT_EQ(top, item.GetTopParent());
}

TEST_F(TestDecorationItem, RelayoutParentOnRequestRelayout)
{
  auto parent = std::make_shared<MockBasicContainer>();
  item.SetParent(parent);

  EXPECT_CALL(*parent, DoRelayout());
  item.RequestRelayout();
}

TEST_F(TestDecorationItem, RelayoutParentOnGeometryChanges)
{
  auto parent = std::make_shared<MockBasicContainer>();
  item.SetParent(parent);

  EXPECT_CALL(*parent, DoRelayout());
  item.geo_parameters_changed.emit();
}

TEST_F(TestDecorationItem, RelayoutParentOnVisibilityChanges)
{
  auto parent = std::make_shared<MockBasicContainer>();
  item.SetParent(parent);

  EXPECT_CALL(*parent, DoRelayout());
  item.visible.changed.emit(false);
}

TEST_F(TestDecorationItem, Geometry)
{
  EXPECT_EQ(item.rect_, item.Geometry());
}

TEST_F(TestDecorationItem, NaturalWidth)
{
  EXPECT_EQ(item.natural_.width, item.GetNaturalWidth());
  EXPECT_EQ(item.Geometry().width(), item.GetNaturalWidth());
}

TEST_F(TestDecorationItem, NaturalHeight)
{
  EXPECT_EQ(item.natural_.height, item.GetNaturalHeight());
  EXPECT_EQ(item.Geometry().height(), item.GetNaturalHeight());
}

TEST_F(TestDecorationItem, SetSize)
{
  int w = random_positive_int(), h = random_positive_int();

  EXPECT_CALL(item, SetMinWidth(w));
  EXPECT_CALL(item, SetMaxWidth(w));
  EXPECT_CALL(item, SetMinHeight(h));
  EXPECT_CALL(item, SetMaxHeight(h));
  EXPECT_CALL(sig_receiver, GeoChanged()).Times(AtLeast(1));

  item.SetSize(w, h);

  EXPECT_EQ(w, item.GetNaturalWidth());
  EXPECT_EQ(h, item.GetNaturalHeight());
}

TEST_F(TestDecorationItem, SetNegativeSize)
{
  item.SetSize(g_random_int_range(G_MININT, 0), g_random_int_range(G_MININT, 0));

  EXPECT_EQ(0, item.GetNaturalWidth());
  EXPECT_EQ(0, item.GetNaturalHeight());
}

TEST_F(TestDecorationItem, SetWidth)
{
  int w = random_positive_int();

  EXPECT_CALL(item, SetSize(w, item.Geometry().height()));
  item.SetWidth(w);

  EXPECT_EQ(w, item.GetNaturalWidth());
  EXPECT_EQ(item.Geometry().height(), item.GetNaturalHeight());
}

TEST_F(TestDecorationItem, SetHeight)
{
  int h = random_positive_int();

  EXPECT_CALL(item, SetSize(item.Geometry().width(), h));
  item.SetHeight(h);

  EXPECT_EQ(item.Geometry().width(), item.GetNaturalWidth());
  EXPECT_EQ(h, item.GetNaturalHeight());
}

TEST_F(TestDecorationItem, SetCoords)
{
  int x = random_int(), y = random_int();

  EXPECT_CALL(sig_receiver, GeoChanged());
  item.SetCoords(x, y);

  EXPECT_EQ(item.Geometry().x(), x);
  EXPECT_EQ(item.Geometry().y(), y);
}

TEST_F(TestDecorationItem, SetX)
{
  int x = random_int();

  EXPECT_CALL(item, SetCoords(x, item.Geometry().y()));
  item.SetX(x);

  EXPECT_EQ(item.Geometry().x(), x);
}

TEST_F(TestDecorationItem, SetY)
{
  int y = random_int();

  EXPECT_CALL(item, SetCoords(item.Geometry().x(), y));
  item.SetY(y);

  EXPECT_EQ(item.Geometry().y(), y);
}

//

struct MockLayout : Layout
{
  typedef std::shared_ptr<MockLayout> Ptr;
  typedef NiceMock<MockLayout> Nice;

  MockLayout()
  {
    ON_CALL(*this, DoRelayout()).WillByDefault(Invoke(this, &MockLayout::LocalRelayout));
  }

  void LocalRelayout() { Layout::DoRelayout(); }
  MOCK_METHOD0(DoRelayout, void());
};

struct TestDecorationLayout : Test
{
  TestDecorationLayout()
    : layout(std::make_shared<MockLayout::Nice>())
  {}

  MockLayout::Ptr layout;
};

TEST_F(TestDecorationLayout, AppendUnlimited)
{
  layout->SetCoords(random_int(), random_int());
  auto expected_geo = layout->Geometry();

  for (int i = 0; i < 100; ++i)
  {
    auto item = RandomMockItem();
    auto const& item_geo = item->Geometry();
    expected_geo.setWidth(expected_geo.width() + item_geo.width());
    expected_geo.setHeight(std::max(expected_geo.height(), item_geo.height()));

    layout->Append(item);
    ASSERT_EQ(expected_geo, layout->Geometry());
    ASSERT_EQ(layout->Geometry().x2() - item_geo.width(), item_geo.x());
    ASSERT_EQ(layout->Geometry().y() + (layout->Geometry().height() - item_geo.height()) / 2, item_geo.y());
  }

  EXPECT_EQ(100, layout->Items().size());
}

TEST_F(TestDecorationLayout, AppendParentsItem)
{
  auto item = RandomMockItem();
  layout->Append(item);
  EXPECT_EQ(layout, item->GetParent());
}

TEST_F(TestDecorationLayout, AppendDontAddParentedItems)
{
  auto item = RandomMockItem();
  layout->Append(item);

  auto layout2 = std::make_shared<Layout>();
  layout2->Append(item);

  EXPECT_EQ(layout, item->GetParent());
  EXPECT_THAT(layout->Items(), Contains(item));
  EXPECT_THAT(layout2->Items(), Not(Contains(item)));
}

TEST_F(TestDecorationLayout, RemoveUnParentsItem)
{
  auto item = RandomMockItem();
  layout->Append(item);
  ASSERT_EQ(layout, item->GetParent());

  layout->Remove(item);
  EXPECT_EQ(nullptr, item->GetParent());
}

TEST_F(TestDecorationLayout, AppendInvisible)
{
  CompRect expected_geo;
  for (int i = 0; i < 100; ++i)
  {
    auto item = RandomMockItem();
    item->visible = false;
    layout->Append(item);
    ASSERT_EQ(expected_geo, layout->Geometry());
  }

  EXPECT_EQ(100, layout->Items().size());
  EXPECT_EQ(CompRect(), layout->Geometry());
}

TEST_F(TestDecorationLayout, AppendUnlimitedInternalPadding)
{
  layout->inner_padding = g_random_int_range(10, 50);
  layout->SetCoords(random_int(), random_int());
  auto expected_geo = layout->Geometry();

  for (int i = 0; i < 100; ++i)
  {
    auto item = RandomMockItem();
    auto const& item_geo = item->Geometry();
    expected_geo.setWidth(expected_geo.width() + item_geo.width() + layout->inner_padding() * ((i > 0) ? 1 : 0));
    expected_geo.setHeight(std::max(expected_geo.height(), item_geo.height()));

    layout->Append(item);
    ASSERT_EQ(expected_geo, layout->Geometry());
    ASSERT_EQ(layout->Geometry().x2() - item_geo.width(), item_geo.x());
    ASSERT_EQ(layout->Geometry().y() + (layout->Geometry().height() - item_geo.height()) / 2, item_geo.y());
  }

  EXPECT_EQ(100, layout->Items().size());
}

TEST_F(TestDecorationLayout, AppendWithMaxWidth)
{
  layout->SetCoords(random_int(), random_int());

  for (int i = 0; i < 100; ++i)
    layout->Append(RandomMockItem());

  ASSERT_EQ(100, layout->Items().size());

  auto const& layout_geo = layout->Geometry();
  int new_width = layout_geo.width()/2;
  layout->SetMaxWidth(new_width);

  ASSERT_EQ(new_width, layout->GetMaxWidth());
  ASSERT_EQ(new_width, layout_geo.width());

  for (auto const& item : layout->Items())
  {
    auto const& item_geo = item->Geometry();

    if (item_geo.x2() < layout_geo.x2())
    {
      ASSERT_EQ(item->GetNaturalWidth(), item_geo.width());
      ASSERT_EQ(item->GetNaturalHeight(), item_geo.height());
    }
    else
    {
      ASSERT_EQ(std::max(0, layout_geo.x2() - item_geo.x1()), item_geo.width());
    }
  }
}

TEST_F(TestDecorationLayout, ExpandWithMaxWidth)
{
  layout->SetCoords(random_int(), random_int());

  for (int i = 0; i < 100; ++i)
    layout->Append(RandomMockItem());

  ASSERT_EQ(100, layout->Items().size());

  auto const& layout_geo = layout->Geometry();
  int full_width = layout_geo.width();

  layout->SetMaxWidth(full_width/2);
  ASSERT_EQ(full_width/2, layout_geo.width());

  layout->SetMaxWidth(std::numeric_limits<int>::max());
  ASSERT_EQ(full_width, layout_geo.width());

  for (auto const& item : layout->Items())
    ASSERT_EQ(item->GetNaturalWidth(), item->Geometry().width());
}

TEST_F(TestDecorationLayout, Focused)
{
  for (int i = 0; i < 100; ++i)
  {
    auto const& item = RandomMockItem();
    layout->Append(item);
    ASSERT_FALSE(item->focused());
  }

  layout->focused = true;

  for (auto const& item : layout->Items())
    ASSERT_TRUE(item->focused());
}

TEST_F(TestDecorationLayout, AddToFocused)
{
  auto const& item = RandomMockItem();
  layout->focused = true;
  ASSERT_FALSE(item->focused());

  layout->Append(item);
  ASSERT_TRUE(item->focused());
}

TEST_F(TestDecorationLayout, Scale)
{
  for (int i = 0; i < 100; ++i)
  {
    auto const& item = RandomMockItem();
    layout->Append(item);
    ASSERT_FLOAT_EQ(1.0f, item->scale());
  }

  layout->scale = 2.0f;

  for (auto const& item : layout->Items())
    ASSERT_FLOAT_EQ(2.0f, item->scale());
}

TEST_F(TestDecorationLayout, AddToScaled)
{
  auto const& item = RandomMockItem();
  layout->scale = 0.5f;
  ASSERT_FLOAT_EQ(1.0f, item->scale());

  layout->Append(item);
  EXPECT_FLOAT_EQ(0.5f, item->scale());
}

TEST_F(TestDecorationLayout, ContentGeo)
{
  layout->Append(RandomMockItem());
  EXPECT_EQ(layout->Geometry(), layout->ContentGeometry());
}

TEST_F(TestDecorationLayout, ContentGeoWithPadding)
{
  layout->Append(RandomMockItem());
  layout->top_padding = random_positive_int();
  layout->left_padding = random_positive_int();
  layout->right_padding = random_positive_int();
  layout->bottom_padding = random_positive_int();

  CompRect expected_geo(layout->Geometry().x() + layout->left_padding(),
                        layout->Geometry().y() + layout->top_padding(),
                        layout->Geometry().width() - layout->left_padding() - layout->right_padding(),
                        layout->Geometry().height() - layout->top_padding() - layout->bottom_padding());
  EXPECT_EQ(expected_geo, layout->ContentGeometry());
}

TEST_F(TestDecorationLayout, RecursivelyRelayoutsOnGeometryChanges)
{
  std::vector<MockLayout::Ptr> containers;
  auto parent = layout;
  containers.push_back(parent);

  for (int i = 0; i < 10; ++i)
  {
    auto c = std::make_shared<MockLayout::Nice>();
    containers.push_back(c);
    parent->Append(c);
    parent = c;
  }

  auto item = RandomMockItem();
  parent->Append(item);

  for (auto const& c : containers)
    EXPECT_CALL(*c, DoRelayout()).Times(AtLeast(1));

  item->SetSize(item->Geometry().width()+1, item->Geometry().height()+1);
}

}
