// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 */

#include <memory>
#include <stdexcept>
#include <gtest/gtest.h>

#include "MockShortcutHint.h"
#include "ShortcutModel.h"

using namespace unity::shortcut;

namespace {

TEST(TestShortcutModel, TestConstruction)
{
  std::list<AbstractHint::Ptr> hints;

  hints.push_back(std::make_shared<MockHint>("Launcher", "", "", "Description 1", OptionType::COMPIZ_KEY, "Plugin 1", "key_option_1"));
  hints.push_back(std::make_shared<MockHint>("Launcher", "", "", "Description 2", OptionType::HARDCODED, "Value 2"));
  hints.push_back(std::make_shared<MockHint>("Dash", "Prefix", "Postfix", "Description 3", OptionType::COMPIZ_KEY, "Plugin 3", "key_option_3"));
  hints.push_back(std::make_shared<MockHint>("Menu Bar", "Prefix", "Postfix", "Description 4", OptionType::HARDCODED, "Value4"));

  Model model(hints);

  EXPECT_EQ(model.categories_per_column(), 3);
  EXPECT_EQ(model.categories().size(), 3);
  EXPECT_EQ(model.hints().at("Launcher").size(), 2);
  EXPECT_EQ(model.hints().at("Dash").size(), 1);
  EXPECT_EQ(model.hints().at("Menu Bar").size(), 1);
  EXPECT_EQ(model.hints().find("Unity"), model.hints().end());
}

TEST(TestShortcutModel, TestFill)
{
  std::list<AbstractHint::Ptr> hints;

  hints.push_back(std::make_shared<MockHint>("Launcher", "", "", "Description 1", OptionType::COMPIZ_KEY, "Plugin 1", "key_option_1"));
  hints.push_back(std::make_shared<MockHint>("Launcher", "", "", "Description 2", OptionType::HARDCODED, "Value 2"));
  hints.push_back(std::make_shared<MockHint>("Dash", "Prefix", "Postfix", "Description 3", OptionType::COMPIZ_KEY, "Plugin 3", "key_option_3"));
  hints.push_back(std::make_shared<MockHint>("Menu Bar", "Prefix", "Postfix", "Description 4", OptionType::HARDCODED, "Value 4"));

  Model model(hints);

  model.Fill();

  // We cannot test CompOption here... :/
  EXPECT_EQ(model.hints().at("Launcher").front()->value(), "Plugin 1-key_option_1");
  EXPECT_EQ(model.hints().at("Launcher").back()->value(), "Value 2");
  EXPECT_EQ(model.hints().at("Dash").front()->value(),"Plugin 3-key_option_3");
  EXPECT_EQ(model.hints().at("Menu Bar").front()->value(), "Value 4");
}

TEST(TestShortcutModel, TestProperty)
{
  std::list<AbstractHint::Ptr> hints;

  hints.push_back(std::make_shared<MockHint>("Launcher", "Prefix1", "Postfix1", "Description1", OptionType::COMPIZ_KEY, "Plugin1", "key_option1"));

  Model model(hints);

  EXPECT_EQ(model.hints().at("Launcher").front()->category(), "Launcher");
  EXPECT_EQ(model.hints().at("Launcher").front()->prefix(), "Prefix1");
  EXPECT_EQ(model.hints().at("Launcher").front()->postfix(), "Postfix1");
  EXPECT_EQ(model.hints().at("Launcher").front()->description(), "Description1");
  EXPECT_EQ(model.hints().at("Launcher").front()->type(), OptionType::COMPIZ_KEY);
  EXPECT_EQ(model.hints().at("Launcher").front()->arg1(), "Plugin1");
  EXPECT_EQ(model.hints().at("Launcher").front()->arg2(), "key_option1");
}

TEST(TestShortcutModel, CategoriesPerColumnSetter)
{
  std::list<AbstractHint::Ptr> hints;
  Model model(hints);

  bool changed = false;
  model.categories_per_column.changed.connect([&changed] (int) {changed = true;});

  model.categories_per_column = 3456789;
  EXPECT_TRUE(changed);
  EXPECT_EQ(model.categories_per_column(), 3456789);

  changed = false;
  model.categories_per_column = 0;
  EXPECT_TRUE(changed);
  EXPECT_EQ(model.categories_per_column(), 1);

  changed = false;
  model.categories_per_column = -1;
  EXPECT_FALSE(changed);
  EXPECT_EQ(model.categories_per_column(), 1);
}

} // anonymouse namespace
