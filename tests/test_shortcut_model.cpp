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

#include <gtest/gtest.h>

#include "MockShortcutHint.h"
#include "ShortcutModel.h"

using namespace unity::shortcut;

namespace {

TEST(TestShortcutModel, TestConstruction)
{
  std::list<AbstractHint*> hints;
  
  hints.push_back(new MockHint("Launcher", "", "", "Description 1", COMPIZ_OPTION, "Plugin 1", "key_option_1"));
  hints.push_back(new MockHint("Launcher", "", "", "Description 2", HARDCODED_OPTION, "Value 2"));
  hints.push_back(new MockHint("Dash", "Prefix", "Postfix", "Description 3", COMPIZ_OPTION, "Plugin 3", "key_option_3"));
  hints.push_back(new MockHint("Top Bar", "Prefix", "Postfix", "Description 4", HARDCODED_OPTION, "Value4"));
  
  Model model(hints);
  
  EXPECT_EQ(model.categories().size(), 3);
  EXPECT_EQ(model.hints()["Launcher"].size(), 2);
  EXPECT_EQ(model.hints()["Dash"].size(), 1);
  EXPECT_EQ(model.hints()["Top Bar"].size(), 1);
  EXPECT_EQ(model.hints()["Unity"].size(), 0);
}

TEST(TestShortcutModel, TestFill)
{
  std::list<AbstractHint*> hints;
  
  hints.push_back(new MockHint("Launcher", "", "", "Description 1", COMPIZ_OPTION, "Plugin 1", "key_option_1"));
  hints.push_back(new MockHint("Launcher", "", "", "Description 2", HARDCODED_OPTION, "Value 2"));
  hints.push_back(new MockHint("Dash", "Prefix", "Postfix", "Description 3", COMPIZ_OPTION, "Plugin 3", "key_option_3"));
  hints.push_back(new MockHint("Top Bar", "Prefix", "Postfix", "Description 4", HARDCODED_OPTION, "Value 4"));
  
  Model model(hints);
  
  model.Fill();
  
  // We cannot test CompOption here... :/
  EXPECT_EQ(model.hints()["Launcher"].front()->value(), "Plugin 1-key_option_1");
  EXPECT_EQ(model.hints()["Launcher"].back()->value(), "Value 2");
  EXPECT_EQ(model.hints()["Dash"].front()->value(),"Plugin 3-key_option_3");
  EXPECT_EQ(model.hints()["Top Bar"].front()->value(), "Value 4");
}

TEST(TestShortcutModel, TestProperty)
{
  std::list<AbstractHint*> hints;
  
  hints.push_back(new MockHint("Launcher", "Prefix1", "Postfix1", "Description1", COMPIZ_OPTION, "Plugin1", "key_option1"));
  
  Model model(hints);
  
  EXPECT_EQ(model.hints()["Launcher"].front()->category(), "Launcher");
  EXPECT_EQ(model.hints()["Launcher"].front()->prefix(), "Prefix1");
  EXPECT_EQ(model.hints()["Launcher"].front()->postfix(), "Postfix1");
  EXPECT_EQ(model.hints()["Launcher"].front()->description(), "Description1");
  EXPECT_EQ(model.hints()["Launcher"].front()->type(), COMPIZ_OPTION);
  EXPECT_EQ(model.hints()["Launcher"].front()->arg1(), "Plugin1");
  EXPECT_EQ(model.hints()["Launcher"].front()->arg2(), "key_option1");  
}

} // anonymouse namespace
