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
#include <glib.h>
#include <memory>
#include <boost/foreach.hpp>

#include "Introspectable.h"
#include "DebugDBusInterface.h"


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
    g_variant_builder_add (builder, "{sv}", "Name", g_variant_new_string (name_.c_str()) );
    g_variant_builder_add (builder, "{sv}", "SomeProperty", g_variant_new_string ("SomeValue") );
    g_variant_builder_add (builder, "{sv}", "BoolPropertyTrue", g_variant_new_boolean (TRUE) );
    g_variant_builder_add (builder, "{sv}", "BoolPropertyFalse", g_variant_new_boolean (FALSE) );
    // 8-bit integer types:
    g_variant_builder_add (builder, "{sv}", "BytePropertyPos", g_variant_new_byte (12) );
    // 16-bit integer types:
    g_variant_builder_add (builder, "{sv}", "Int16PropertyPos", g_variant_new_int16 (1012) );
    g_variant_builder_add (builder, "{sv}", "Int16PropertyNeg", g_variant_new_int16 (-1034) );
    g_variant_builder_add (builder, "{sv}", "UInt16PropertyPos", g_variant_new_uint16 (1056) );
    // 32-bit integer types:
    g_variant_builder_add (builder, "{sv}", "Int32PropertyPos", g_variant_new_int32 (100012) );
    g_variant_builder_add (builder, "{sv}", "Int32PropertyNeg", g_variant_new_int32 (-100034) );
    g_variant_builder_add (builder, "{sv}", "UInt32PropertyPos", g_variant_new_uint32 (100056) );
    // 64-bit integer types
    g_variant_builder_add (builder, "{sv}", "Int64PropertyPos", g_variant_new_int32 (100000012) );
    g_variant_builder_add (builder, "{sv}", "Int64PropertyNeg", g_variant_new_int32 (-100000034) );
    g_variant_builder_add (builder, "{sv}", "UInt64PropertyPos", g_variant_new_uint32 (100000056) );

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

    //root_->SetProperty(g_variant_new("{sv}", "SomeProperty", g_variant_new_string("SomeValue")));
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

  results = GetIntrospectableNodesFromQuery(query, root_.get());
  ASSERT_EQ(1, results.size());
  EXPECT_STREQ("Unity", results.front()->GetName().c_str());

  query = "/";
  results = GetIntrospectableNodesFromQuery(query, root_.get());
  ASSERT_EQ(1, results.size());
  EXPECT_STREQ("Unity", results.front()->GetName().c_str());

  query = "/Unity";
  results = GetIntrospectableNodesFromQuery(query, root_.get());
  ASSERT_EQ(1, results.size());
  EXPECT_STREQ("Unity", results.front()->GetName().c_str());
}

TEST_F(TestIntrospection, TestAsteriskWildcard)
{
  std::list<Introspectable*> results;
  std::string query = "/Unity/*";

  results = GetIntrospectableNodesFromQuery(query, root_.get());
  ASSERT_EQ(2, results.size());

  for(auto p : results)
  {
    ASSERT_TRUE(
      p->GetName() == "DashController" ||
      p->GetName() == "PanelController"
      );
  }
}

TEST_F(TestIntrospection, TestRelativeAsteriskWildcard)
{
  std::list<Introspectable*> results;
  std::string query = "//DashController/*";

  results = GetIntrospectableNodesFromQuery(query, root_.get());
  ASSERT_EQ(3, results.size());

  for(auto p : results)
  {
    ASSERT_TRUE(p->GetName() == "Foo");
  }
}

TEST_F(TestIntrospection, TestAbsoluteQueries)
{
  std::list<Introspectable*> results;
  std::string query = "/Unity/DashController";

  results = GetIntrospectableNodesFromQuery(query, root_.get());
  ASSERT_EQ(1, results.size());
  EXPECT_STREQ("DashController", results.front()->GetName().c_str());
}

TEST_F(TestIntrospection, TestMalformedRelativeQueries)
{
  std::list<Introspectable*> results;
  std::string query = "Unity";

  results = GetIntrospectableNodesFromQuery(query, root_.get());
  ASSERT_EQ(1, results.size());
  EXPECT_STREQ("Unity", results.front()->GetName().c_str());

  query = "Foo";
  results = GetIntrospectableNodesFromQuery(query, root_.get());
  ASSERT_EQ(3, results.size());
  for(auto p : results)
  {
    EXPECT_STREQ("Foo", p->GetName().c_str());
  }
}

TEST_F(TestIntrospection, TestSimpleRelativeQueries)
{
  std::list<Introspectable*> results;
  std::string query = "//Unity";

  results = GetIntrospectableNodesFromQuery(query, root_.get());
  ASSERT_EQ(1, results.size());
  EXPECT_STREQ("Unity", results.front()->GetName().c_str());

  query = "//Foo";
  results = GetIntrospectableNodesFromQuery(query, root_.get());
  ASSERT_EQ(3, results.size());
  for(auto p : results)
  {
    EXPECT_STREQ("Foo", p->GetName().c_str());
  }
}

TEST_F(TestIntrospection, TestComplexRelativeQueries)
{
  std::list<Introspectable*> results;
  std::string query = "//DashController/Foo";

  results = GetIntrospectableNodesFromQuery(query, root_.get());
  ASSERT_EQ(3, results.size());
  for(auto p : results)
  {
    EXPECT_STREQ("Foo", p->GetName().c_str());
  }
}

TEST_F(TestIntrospection, TestQueriesWithNoResults)
{
  std::list<Introspectable*> results;
  std::string query = "//Does/Not/Exist";

  results = GetIntrospectableNodesFromQuery(query, root_.get());
  ASSERT_EQ(0, results.size());

  query = "DoesNotEverExist";
  results = GetIntrospectableNodesFromQuery(query, root_.get());
  ASSERT_EQ(0, results.size());

  query = "/Does/Not/Ever/Exist";
  results = GetIntrospectableNodesFromQuery(query, root_.get());
  ASSERT_EQ(0, results.size());
}

TEST_F(TestIntrospection, TestQueriesWithParams)
{
  std::list<Introspectable*> results;
  // this should find our root node:
  results = GetIntrospectableNodesFromQuery("/Unity[SomeProperty=SomeValue]", root_.get());
  ASSERT_EQ(1, results.size());
  EXPECT_STREQ("Unity", results.front()->GetName().c_str());
  // but this should find nothing:
  results = GetIntrospectableNodesFromQuery("/Unity[SomeProperty=SomeOtherValue]", root_.get());
  ASSERT_EQ(0, results.size());

  // make sure relative paths work:
  results = GetIntrospectableNodesFromQuery("//Foo[Name=Foo]", root_.get());
  ASSERT_EQ(3, results.size());
  for(auto p : results)
  {
    EXPECT_STREQ("Foo", p->GetName().c_str());
  }

  // make sure param queries work with descendant nodes as well:
  results = GetIntrospectableNodesFromQuery("/Unity[SomeProperty=SomeValue]/DashController[Name=DashController]/Foo", root_.get());
  ASSERT_EQ(3, results.size());
  for(auto p : results)
  {
    EXPECT_STREQ("Foo", p->GetName().c_str());
  }
}

TEST_F(TestIntrospection, TestQueryTypeBool)
{
  std::list<Introspectable*> results;

  // These are all equivilent and should return the root item and nothing more:
  std::list<std::string> queries = {"/Unity[BoolPropertyTrue=True]",
                                    "/Unity[BoolPropertyTrue=true]",
                                    "/Unity[BoolPropertyTrue=trUE]",
                                    "/Unity[BoolPropertyTrue=yes]",
                                    "/Unity[BoolPropertyTrue=ON]",
                                    "/Unity[BoolPropertyTrue=1]"};

  for(auto query : queries)
  {
    results = GetIntrospectableNodesFromQuery(query, root_.get());
    ASSERT_EQ(1, results.size());
    EXPECT_STREQ("Unity", results.front()->GetName().c_str());
  }

  // For boolean properties, anything that's not True, Yes, On or 1 is treated as false:
  queries = {"/Unity[BoolPropertyTrue=False]",
            "/Unity[BoolPropertyTrue=fAlSE]",
            "/Unity[BoolPropertyTrue=No]",
            "/Unity[BoolPropertyTrue=OFF]",
            "/Unity[BoolPropertyTrue=0]",
            "/Unity[BoolPropertyTrue=ThereWasAManFromNantucket]"};
  for(auto query : queries)
  {
    results = GetIntrospectableNodesFromQuery(query, root_.get());
    ASSERT_EQ(0, results.size());
  }
}

TEST_F(TestIntrospection, TestQueryTypeInt)
{
  std::list<Introspectable*> results;

  // these should all select the root Unity node:
  std::list<std::string> queries = {"/Unity[BytePropertyPos=12]",
                                    "/Unity[Int16PropertyPos=1012]",
                                    "/Unity[Int16PropertyNeg=-1034]",
                                    "/Unity[UInt16PropertyPos=1056]",
                                    "/Unity[Int32PropertyPos=100012]",
                                    "/Unity[Int32PropertyNeg=-100034]",
                                    "/Unity[UInt32PropertyPos=100056]",
                                    "/Unity[Int64PropertyPos=100000012]",
                                    "/Unity[Int64PropertyNeg=-100000034]",
                                    "/Unity[UInt64PropertyPos=100000056]"};
  for(auto query : queries)
  {
    results = GetIntrospectableNodesFromQuery(query, root_.get());
    ASSERT_EQ(1, results.size()) << "Failing query: " << query;
    EXPECT_STREQ("Unity", results.front()->GetName().c_str());
  }

  // but these shouldn't:
  queries = {"/Unity[BytePropertyPos=1234]",
            "/Unity[Int16PropertyPos=0]",
            "/Unity[Int16PropertyNeg=-0]",
            "/Unity[Int16PropertyNeg=-]",
            "/Unity[UInt16PropertyPos=-1056]",
            "/Unity[Int32PropertyPos=999999999999999]",
            "/Unity[Int32PropertyNeg=Garbage]",
            "/Unity[UInt32PropertyPos=-23]"};
  for(auto query : queries)
  {
    results = GetIntrospectableNodesFromQuery(query, root_.get());
    ASSERT_EQ(0, results.size());
  }
}

TEST_F(TestIntrospection, TestMalformedQueries)
{
  // this should work - we have not yet specified a parameter to test against.
  std::list<Introspectable*> results = GetIntrospectableNodesFromQuery("/Unity[", root_.get());
  ASSERT_EQ(1, results.size());

  std::list<std::string> queries = {"/Unity[BoolPropertyTrue",
                                  "/Unity[BoolPropertyTrue=",
                                  "/Unity[BoolPropertyTrue=]",
                                  "/Unity[BytePropertyPos=",
                                  "/Unity[BytePropertyPos=]",
                                  "/Unity[Int16PropertyPos=",
                                  "/Unity[Int16PropertyPos=]",
                                  "/Unity[Int16PropertyNeg=",
                                  "/Unity[Int16PropertyNeg=]",
                                  "/Unity[UInt16PropertyPos[=]]",
                                  "/Unity[Int32PropertyPos[[",
                                  "/Unity[Int32PropertyNeg]",
                                  "/Unity[UInt32PropertyPos=[",
                                  "/Unity[Int64PropertyPos[[",
                                  "/Unity[Int64PropertyNeg",
                                  "/Unity[UInt64PropertyPos]"};

  for (std::string query : queries)
  {
    results = GetIntrospectableNodesFromQuery(query, root_.get());
    ASSERT_EQ(0, results.size()) << "Failing query: " << query;
  }
}
