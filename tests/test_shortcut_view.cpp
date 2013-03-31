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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>
using namespace testing;

#include "ShortcutView.h"
#include "ShortcutModel.h"
#include "MockShortcutHint.h"
#include "UnitySettings.h"
#include "LineSeparator.h"

namespace unity
{
namespace shortcut
{
struct TestShortcutView : Test
{
  struct MockShortcutView : View
  {
    public:
      MockShortcutView()
        : View(nullptr)
      {
      }

      using View::columns_layout_;
  };

  Model::Ptr GetMockModel(std::vector<std::string> const& categories, unsigned number)
  {
    std::list<AbstractHint::Ptr> hints;

    for (auto const& category : categories)
    {
      for (unsigned i = 0; i < number; ++i)
      {
        auto str_id = category + std::to_string(i);
        auto hint = std::make_shared<MockHint>(category, "", "", "Description "+str_id, OptionType::HARDCODED, "Value "+str_id);
        hints.push_back(hint);
      }
    }

    return std::make_shared<Model>(hints);
  }

  Settings settings;
  MockShortcutView view;
};

TEST_F(TestShortcutView, Construction)
{
  EXPECT_NE(view.GetLayout(), nullptr);
  auto const& children = view.GetLayout()->GetChildren();
  EXPECT_NE(std::find(children.begin(), children.end(), view.columns_layout_), children.end());
}

TEST_F(TestShortcutView, SetModel)
{
  auto model = GetMockModel({}, 0);
  view.SetModel(model);
  EXPECT_EQ(view.GetModel(), model);
  EXPECT_FALSE(model->categories_per_column.changed.empty());
}

TEST_F(TestShortcutView, SetNullModel)
{
  view.SetModel(GetMockModel({}, 0));
  view.SetModel(nullptr);
  EXPECT_EQ(view.GetModel(), nullptr);
  EXPECT_TRUE(view.columns_layout_->GetChildren().empty());
}

TEST_F(TestShortcutView, SettingModelAddsColumns)
{
  auto model = GetMockModel({"Cat1", "Cat2"}, 1);
  model->categories_per_column = 1;
  view.SetModel(model);

  EXPECT_EQ(view.columns_layout_->GetChildren().size(), 2);
}

TEST_F(TestShortcutView, SettingModelRebuildsColumns)
{
  auto model1 = GetMockModel({"Cat1", "Cat2"}, 1);
  model1->categories_per_column = 1;
  view.SetModel(model1);
  ASSERT_EQ(view.columns_layout_->GetChildren().size(), 2);

  auto model2 = GetMockModel({"Cat1"}, 1);
  model2->categories_per_column = 1;
  view.SetModel(model2);
  EXPECT_EQ(view.columns_layout_->GetChildren().size(), 1);
}

TEST_F(TestShortcutView, ChangingModelParametersRebuildsColumns)
{
  auto model = GetMockModel({"Cat1", "Cat2", "Cat3", "Cat4", "Cat5"}, 1);
  model->categories_per_column = std::numeric_limits<int>::max();
  view.SetModel(model);

  for (unsigned i = 1; i <= model->categories().size() * 10; ++i)
  {
    model->categories_per_column = i;
    int expected_columns = std::ceil(model->categories().size() / static_cast<double>(model->categories_per_column));
    ASSERT_EQ(view.columns_layout_->GetChildren().size(), expected_columns);

    int added_cats = 0;
    for (auto col : view.columns_layout_->GetChildren())
    {
      int expected_cats = model->categories_per_column();

      if (expected_cats > int(model->categories().size()) ||
          added_cats == (expected_columns - 1) * model->categories_per_column)
      {
        expected_cats = model->categories().size() - added_cats;
      }

      ASSERT_EQ(dynamic_cast<nux::Layout*>(col)->GetChildren().size(), expected_cats);
      added_cats += expected_cats;
    }
  }
}

TEST_F(TestShortcutView, CategoryHasSeparator)
{
  auto model = GetMockModel({"Cat1", "Cat2", "Cat3", "Cat4", "Cat5"}, 1);
  model->categories_per_column = std::numeric_limits<int>::max();
  view.SetModel(model);

  for (unsigned i = 1; i <= model->categories().size(); ++i)
  {
    model->categories_per_column = i;

    for (auto col : view.columns_layout_->GetChildren())
    {
      auto column = dynamic_cast<nux::Layout*>(col);

      for (auto section : column->GetChildren())
      {
        unsigned found_separators = 0;
        for (auto item : dynamic_cast<nux::Layout*>(section)->GetChildren())
          if (dynamic_cast<HSeparator*>(item))
            ++found_separators;

        ASSERT_EQ(found_separators, section != column->GetChildren().back() ? 1 : 0);
      }
    }

  }
}

TEST_F(TestShortcutView, QueueRelayoutOnHintChange)
{
  auto model = GetMockModel({"Cat"}, 1);
  view.SetModel(model);

  // Removing any queued relayout from WT, so that we can check if we queue a new one
  nux::GetWindowThread()->RemoveObjectFromLayoutQueue(&view);
  ASSERT_FALSE(nux::GetWindowThread()->RemoveObjectFromLayoutQueue(&view));

  // Changing a shortkey should queue a relayout
  model->hints().at("Cat").front()->shortkey.changed.emit("New Key!");
  EXPECT_TRUE(nux::GetWindowThread()->RemoveObjectFromLayoutQueue(&view));
}

TEST_F(TestShortcutView, HintSignalsDisconnectedOnCleanup)
{
  AbstractHint::Ptr hint;
  {
    MockShortcutView mock_view;
    mock_view.SetModel(GetMockModel({"Cat"}, 1));
    hint = mock_view.GetModel()->hints().at("Cat").front();
    ASSERT_FALSE(hint->shortkey.changed.empty());
  }

  ASSERT_TRUE(hint->shortkey.changed.empty());
  hint->shortkey.changed.emit("New Key!");
}

TEST_F(TestShortcutView, HintSignalsDisconnectedOnModelReplace)
{
  AbstractHint::Ptr hint;
  view.SetModel(GetMockModel({"Cat"}, 1));
  hint = view.GetModel()->hints().at("Cat").front();
  ASSERT_FALSE(hint->shortkey.changed.empty());

  // Replacing the model with a new one
  view.SetModel(GetMockModel({"Cat"}, 1));

  ASSERT_TRUE(hint->shortkey.changed.empty());
  hint->shortkey.changed.emit("New Key!");
}

}
}
