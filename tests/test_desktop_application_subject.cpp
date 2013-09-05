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
 *
 */

#include <gtest/gtest.h>
#include <unity-shared/DesktopApplicationManager.h>
#include "mock-application.h"

using namespace unity::desktop;

namespace
{

struct TestDestkopApplicationSubject : testing::Test
{
  ApplicationSubject subject;
};

struct Property : TestDestkopApplicationSubject, testing::WithParamInterface<std::string> {};
INSTANTIATE_TEST_CASE_P(TestDestkopApplicationSubject, Property, testing::Values("Fooo", "Bar", "Unity"));

TEST_P(/*TestDesktopApplicationSubject*/Property, Uri)
{
  ASSERT_TRUE(subject.uri().empty());

  bool changed = false;
  subject.uri.changed.connect([&changed] (std::string const&) { changed = true; });
  subject.uri = GetParam();
  ASSERT_TRUE(changed);
  ASSERT_EQ(GetParam(), subject.uri());

  changed = false;
  subject.uri = GetParam();
  EXPECT_FALSE(changed);
}

TEST_P(/*TestDesktopApplicationSubject*/Property, Origin)
{
  ASSERT_TRUE(subject.origin().empty());

  bool changed = false;
  subject.origin.changed.connect([&changed] (std::string const&) { changed = true; });
  subject.origin = GetParam();
  ASSERT_TRUE(changed);
  ASSERT_EQ(GetParam(), subject.origin());

  changed = false;
  subject.origin = GetParam();
  EXPECT_FALSE(changed);
}

TEST_P(/*TestDesktopApplicationSubject*/Property, Text)
{
  ASSERT_TRUE(subject.text().empty());

  bool changed = false;
  subject.text.changed.connect([&changed] (std::string const&) { changed = true; });
  subject.text = GetParam();
  ASSERT_TRUE(changed);
  ASSERT_EQ(GetParam(), subject.text());

  changed = false;
  subject.text = GetParam();
  EXPECT_FALSE(changed);
}

TEST_P(/*TestDesktopApplicationSubject*/Property, Storage)
{
  ASSERT_TRUE(subject.storage().empty());

  bool changed = false;
  subject.storage.changed.connect([&changed] (std::string const&) { changed = true; });
  subject.storage = GetParam();
  ASSERT_TRUE(changed);
  ASSERT_EQ(GetParam(), subject.storage());

  changed = false;
  subject.storage = GetParam();
  EXPECT_FALSE(changed);
}

TEST_P(/*TestDesktopApplicationSubject*/Property, CurrentUri)
{
  ASSERT_TRUE(subject.current_uri().empty());

  bool changed = false;
  subject.current_uri.changed.connect([&changed] (std::string const&) { changed = true; });
  subject.current_uri = GetParam();
  ASSERT_TRUE(changed);
  ASSERT_EQ(GetParam(), subject.current_uri());

  changed = false;
  subject.current_uri = GetParam();
  EXPECT_FALSE(changed);
}

TEST_P(/*TestDesktopApplicationSubject*/Property, CurrentOrigin)
{
  ASSERT_TRUE(subject.current_origin().empty());

  bool changed = false;
  subject.current_origin.changed.connect([&changed] (std::string const&) { changed = true;});
  subject.current_origin = GetParam();
  ASSERT_TRUE(changed);
  ASSERT_EQ(GetParam(), subject.current_origin());

  changed = false;
  subject.current_origin = GetParam();
  EXPECT_FALSE(changed);
}

TEST_P(/*TestDesktopApplicationSubject*/Property, Mimetype)
{
  ASSERT_TRUE(subject.mimetype().empty());

  bool changed = false;
  subject.mimetype.changed.connect([&changed] (std::string const&) { changed = true; });
  subject.mimetype = GetParam();
  ASSERT_TRUE(changed);
  ASSERT_EQ(GetParam(), subject.mimetype());

  changed = false;
  subject.mimetype = GetParam();
  EXPECT_FALSE(changed);
}

TEST_P(/*TestDesktopApplicationSubject*/Property, Interpretation)
{
  ASSERT_TRUE(subject.interpretation().empty());

  bool changed = false;
  subject.interpretation.changed.connect([&changed] (std::string const&) { changed = true;});
  subject.interpretation = GetParam();
  ASSERT_TRUE(changed);
  ASSERT_EQ(GetParam(), subject.interpretation());

  changed = false;
  subject.interpretation = GetParam();
  EXPECT_FALSE(changed);
}

TEST_P(/*TestDesktopApplicationSubject*/Property, Manifestation)
{
  ASSERT_TRUE(subject.manifestation().empty());

  bool changed = false;
  subject.manifestation.changed.connect([&changed] (std::string const&) { changed = true; });
  subject.manifestation = GetParam();
  ASSERT_TRUE(changed);
  ASSERT_EQ(GetParam(), subject.manifestation());

  changed = false;
  subject.manifestation = GetParam();
  EXPECT_FALSE(changed);
}

TEST_P(/*TestDesktopApplicationSubject*/Property, EqualityOperator)
{
  subject.uri = GetParam() + "uri";
  subject.origin = GetParam() + "origin";
  subject.text = GetParam() + "text";
  subject.storage = GetParam() + "storage";
  subject.current_uri = GetParam() + "current_uri";
  subject.current_origin = GetParam() + "current_origin";
  subject.mimetype = GetParam() + "mimetype";
  subject.interpretation = GetParam() + "interpretation";
  subject.manifestation = GetParam() + "manifestation";

  ApplicationSubject copy_subject;
  copy_subject.uri = subject.uri();
  copy_subject.origin = subject.origin();
  copy_subject.text = subject.text();
  copy_subject.storage = subject.storage();
  copy_subject.current_uri = subject.current_uri();
  copy_subject.current_origin = subject.current_origin();
  copy_subject.mimetype = subject.mimetype();
  copy_subject.interpretation = subject.interpretation();
  copy_subject.manifestation = subject.manifestation();

  ASSERT_EQ(subject.uri(), copy_subject.uri());
  ASSERT_EQ(subject.origin(), copy_subject.origin());
  ASSERT_EQ(subject.text(), copy_subject.text());
  ASSERT_EQ(subject.storage(), copy_subject.storage());
  ASSERT_EQ(subject.current_uri(), copy_subject.current_uri());
  ASSERT_EQ(subject.current_origin(), copy_subject.current_origin());
  ASSERT_EQ(subject.mimetype(), copy_subject.mimetype());
  ASSERT_EQ(subject.interpretation(), copy_subject.interpretation());
  ASSERT_EQ(subject.manifestation(), copy_subject.manifestation());

  EXPECT_EQ(subject, copy_subject);
}

TEST_P(/*TestDesktopApplicationSubject*/Property, NotEqualityOperator)
{
  subject.uri = GetParam() + "uri";
  subject.origin = GetParam() + "origin";
  subject.text = GetParam() + "text";
  subject.storage = GetParam() + "storage";
  subject.current_uri = GetParam() + "current_uri";
  subject.current_origin = GetParam() + "current_origin";
  subject.mimetype = GetParam() + "mimetype";
  subject.interpretation = GetParam() + "interpretation";
  subject.manifestation = GetParam() + "manifestation";

  ApplicationSubject other_subject;
  other_subject.uri = subject.uri() + "other";
  other_subject.origin = subject.origin() + "other";
  other_subject.text = subject.text() + "other";
  other_subject.storage = subject.storage() + "other";
  other_subject.current_uri = subject.current_uri() + "other";
  other_subject.current_origin = subject.current_origin() + "other";
  other_subject.mimetype = subject.mimetype() + "other";
  other_subject.interpretation = subject.interpretation() + "other";
  other_subject.manifestation = subject.manifestation() + "other";

  ASSERT_NE(subject.uri(), other_subject.uri());
  ASSERT_NE(subject.origin(), other_subject.origin());
  ASSERT_NE(subject.text(), other_subject.text());
  ASSERT_NE(subject.storage(), other_subject.storage());
  ASSERT_NE(subject.current_uri(), other_subject.current_uri());
  ASSERT_NE(subject.current_origin(), other_subject.current_origin());
  ASSERT_NE(subject.mimetype(), other_subject.mimetype());
  ASSERT_NE(subject.interpretation(), other_subject.interpretation());
  ASSERT_NE(subject.manifestation(), other_subject.manifestation());

  EXPECT_NE(subject, other_subject);
}

TEST_P(/*TestDesktopApplicationSubject*/Property, CopyConstructor)
{
  subject.uri = GetParam() + "uri";
  subject.origin = GetParam() + "origin";
  subject.text = GetParam() + "text";
  subject.storage = GetParam() + "storage";
  subject.current_uri = GetParam() + "current_uri";
  subject.current_origin = GetParam() + "current_origin";
  subject.mimetype = GetParam() + "mimetype";
  subject.interpretation = GetParam() + "interpretation";
  subject.manifestation = GetParam() + "manifestation";

  ApplicationSubject copy_subject(subject);
  EXPECT_EQ(subject, copy_subject);
}

TEST_P(/*TestDesktopApplicationSubject*/Property, CopyBaseTypeConstructor)
{
  testmocks::MockApplicationSubject mock_subject;
  mock_subject.uri = GetParam() + "uri";
  mock_subject.origin = GetParam() + "origin";
  mock_subject.text = GetParam() + "text";
  mock_subject.storage = GetParam() + "storage";
  mock_subject.current_uri = GetParam() + "current_uri";
  mock_subject.current_origin = GetParam() + "current_origin";
  mock_subject.mimetype = GetParam() + "mimetype";
  mock_subject.interpretation = GetParam() + "interpretation";
  mock_subject.manifestation = GetParam() + "manifestation";

  ApplicationSubject copy_subject(mock_subject);
  EXPECT_EQ(mock_subject, copy_subject);
}

} // anonymous namespace
