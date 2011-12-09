// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Thomi Richards <thomi.richards@canonical.com>
 */
#include <gtest/gtest.h>

#include "Introspectable.h"
#include "DebugDBusInterface.h"
#include "PlacesSimpleTile.h"


using namespace unity::debug;

class MockIntrospectable : public Introspectable
{
public:
  MockIntrospectable(std::string const& name)
  : name_(name)
  {}

  std::string GetName() const
  {
	return name_;
  }
  void AddProperties(GVariantBuilder* builder)
  {
  }
private:
  std::string name_;
};

class TestIntrospection : public ::testing::Test
{
public:
  TestIntrospection()
  : root_(new MockIntrospectable("Unity")),
  dc_(new MockIntrospectable("DashController")),
  pc_(new MockIntrospectable("PanelController")),
  foo1_(new MockIntrospectable("Foo")),
  foo2_(new MockIntrospectable("Foo")),
  foo3_(new MockIntrospectable("Foo"))
  {
    root_->AddChild(dc_.get());
    root_->AddChild(pc_.get());
    dc_->AddChild(foo1_.get());
    dc_->AddChild(foo2_.get());
    dc_->AddChild(foo3_.get());
  }

protected:
  std::shared_ptr<MockIntrospectable> root_;
  std::shared_ptr<MockIntrospectable> dc_;
  std::shared_ptr<MockIntrospectable> pc_;
  std::shared_ptr<MockIntrospectable> foo1_;
  std::shared_ptr<MockIntrospectable> foo2_;
  std::shared_ptr<MockIntrospectable> foo3_;

};

TEST_F(TestIntrospection, TestTest) 
{
  ASSERT_STREQ("Unity", root_->GetName().c_str());
}

TEST_F(TestIntrospection, TestVariousRootQueries) 
{
  std::list<Introspectable*> results;
  std::string query;

  results = FindQueryStartPoints(query, root_.get());
  ASSERT_EQ(1, results.size());
  EXPECT_STREQ("Unity", results.front()->GetName().c_str());

  query = "/";  
  results = FindQueryStartPoints(query, root_.get());
  ASSERT_EQ(1, results.size());
  EXPECT_STREQ("Unity", results.front()->GetName().c_str());

  query = "/Unity";
  results = FindQueryStartPoints(query, root_.get());
  ASSERT_EQ(1, results.size());
  EXPECT_STREQ("Unity", results.front()->GetName().c_str());
}

TEST_F(TestIntrospection, TestAbsoluteQueries)
{
  std::list<Introspectable*> results;
  std::string query = "/Unity/DashController";
  
  results = FindQueryStartPoints(query, root_.get());
  ASSERT_EQ(1, results.size());
  EXPECT_STREQ("DashController", results.front()->GetName().c_str());
}