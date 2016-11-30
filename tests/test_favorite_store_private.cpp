/*
 * Copyright 2011 Canonical Ltd.
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
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 *
 */

#include <gtest/gtest.h>
 
#include "FavoriteStorePrivate.h"

using namespace unity;

TEST(TestFavoriteStorePrivate, TestGetNewbies)
{
  std::list<std::string> old;
  std::list<std::string> fresh;
  std::vector<std::string> result;
  
  old.push_back("a");
  old.push_back("b");
  old.push_back("c");
  old.push_back("d");
  
  // No change.
  fresh.push_back("a");
  fresh.push_back("b");
  fresh.push_back("c");
  fresh.push_back("d");
  
  result = internal::impl::GetNewbies(old, fresh);
  
  EXPECT_TRUE(result.empty());
  
  // Permutation.
  fresh.clear();
  result.clear();
  fresh.push_back("a");
  fresh.push_back("c");
  fresh.push_back("b");
  fresh.push_back("d");
  
  result = internal::impl::GetNewbies(old, fresh);
  
  EXPECT_TRUE(result.empty());
  
  // a b c d -> a c b
  fresh.clear();
  result.clear();
  fresh.push_back("a");
  fresh.push_back("c");
  fresh.push_back("b");
  
  result = internal::impl::GetNewbies(old, fresh);
        
  EXPECT_TRUE(result.empty());
  
  // a b c d -> a b c d e f
  fresh.clear();
  result.clear();
  fresh.push_back("a");
  fresh.push_back("b");
  fresh.push_back("c");
  fresh.push_back("d");
  fresh.push_back("e");
  fresh.push_back("f");
  
  result = internal::impl::GetNewbies(old, fresh);
  
  EXPECT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0], "e");
  EXPECT_EQ(result[1], "f");
  
  // a b c d -> a b c e f
  fresh.clear();
  result.clear();
  fresh.push_back("a");
  fresh.push_back("b");
  fresh.push_back("c");
  fresh.push_back("e");
  fresh.push_back("f");
  
  result = internal::impl::GetNewbies(old, fresh);
  
  EXPECT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0], "e");
  EXPECT_EQ(result[1], "f");
}

TEST(TestFavoriteStorePrivate, TestGetSignalAddedInfo)
{
  std::list<std::string> favs;
  std::vector<std::string> newbies;
  std::string position;
  bool before;
  
  favs.push_back("a");
  favs.push_back("b");
  favs.push_back("c");
  favs.push_back("d");
  favs.push_back("e");
  
  // b c d e -> a b c d e
  newbies.push_back("a");
  internal::impl::GetSignalAddedInfo(favs, newbies, "a", position, before);
  EXPECT_TRUE(before);
  EXPECT_EQ(position, "b");
  
  // a c d e -> a b c d e
  newbies.clear();
  newbies.push_back("b");
  internal::impl::GetSignalAddedInfo(favs, newbies, "b", position, before);
  EXPECT_FALSE(before);
  EXPECT_EQ(position, "a");
  
  // a b d e -> a b c d e
  newbies.clear();
  newbies.push_back("c");
  internal::impl::GetSignalAddedInfo(favs, newbies, "c", position, before);
  EXPECT_FALSE(before);
  EXPECT_EQ(position, "b");
  
  // a b c e -> a b c d e
  newbies.clear();
  newbies.push_back("d");
  internal::impl::GetSignalAddedInfo(favs, newbies, "d", position, before);
  EXPECT_FALSE(before);
  EXPECT_EQ(position, "c");
  
  // a b c d -> a b c d e
  newbies.clear();
  newbies.push_back("e");
  internal::impl::GetSignalAddedInfo(favs, newbies, "e", position, before);
  EXPECT_FALSE(before);
  EXPECT_EQ(position, "d");
  
  // -> b a c
  favs.clear();
  favs.push_back("b");
  favs.push_back("a");
  favs.push_back("c");
  newbies.clear();
  newbies.push_back("a");
  newbies.push_back("b");
  newbies.push_back("c");
  
  internal::impl::GetSignalAddedInfo(favs, newbies, "b", position, before);
  EXPECT_TRUE(before);
  EXPECT_EQ(position, "");
  
  internal::impl::GetSignalAddedInfo(favs, newbies, "a", position, before);
  EXPECT_FALSE(before);
  EXPECT_EQ(position, "b");
  
  internal::impl::GetSignalAddedInfo(favs, newbies, "c", position, before);
  EXPECT_FALSE(before);
  EXPECT_EQ(position, "a");
}


TEST(TestFavoriteStorePrivate, TestGetRemoved)
{
  std::list<std::string> old;
  std::list<std::string> fresh;
  std::vector<std::string> result;
  
  old.push_back("a");
  old.push_back("b");
  old.push_back("c");
  old.push_back("d");
  
  // No change.
  fresh.push_back("a");
  fresh.push_back("b");
  fresh.push_back("c");
  fresh.push_back("d");
  
  result = internal::impl::GetRemoved(old, fresh);
  
  EXPECT_TRUE(result.empty());
  
  // Permutation.
  fresh.clear();
  result.clear();
  fresh.push_back("a");
  fresh.push_back("c");
  fresh.push_back("b");
  fresh.push_back("d");
  
  result = internal::impl::GetRemoved(old, fresh);
  
  EXPECT_TRUE(result.empty());
  
  // a b c d -> b c
  fresh.clear();
  result.clear();
  fresh.push_back("b");
  fresh.push_back("c");
  
  result = internal::impl::GetRemoved(old, fresh);
  
  EXPECT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0], "a");
  EXPECT_EQ(result[1], "d");
  
  // a b c d -> a e f d
  fresh.clear();
  result.clear();
  fresh.push_back("a");
  fresh.push_back("e");
  fresh.push_back("f");
  fresh.push_back("d");
  
  
  result = internal::impl::GetRemoved(old, fresh);
  
  EXPECT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0], "b");
  EXPECT_EQ(result[1], "c");
}
  

TEST(TestFavoriteStorePrivate, TestNeedToBeReorderedBasic)
{
  std::list<std::string> old;
  std::list<std::string> fresh;
  
  old.push_back("a");
  old.push_back("b");
  old.push_back("c");
  old.push_back("d");
  
  // No change.
  fresh.push_back("a");
  fresh.push_back("b");
  fresh.push_back("c");
  fresh.push_back("d");
  
  EXPECT_FALSE(internal::impl::NeedToBeReordered(old, fresh));
  
  // Permutation.
  fresh.clear();
  fresh.push_back("a");
  fresh.push_back("c");
  fresh.push_back("b");
  fresh.push_back("d");
  
  EXPECT_TRUE(internal::impl::NeedToBeReordered(old, fresh));
  
  // Empty.
  fresh.clear();
  
  EXPECT_FALSE(internal::impl::NeedToBeReordered(old, fresh));
}
 
TEST(TestFavoriteStorePrivate, TestNeedToBeReorderedLess)
{
  std::list<std::string> old;
  std::list<std::string> fresh;
  
  old.push_back("a");
  old.push_back("b");
  old.push_back("c");
  old.push_back("d");
   
  // a b c d -> a b c
  fresh.clear();
  fresh.push_back("a");
  fresh.push_back("b");
  fresh.push_back("c");
  
  EXPECT_FALSE(internal::impl::NeedToBeReordered(old, fresh));
  
  // a b c d ->  b c d
  fresh.clear();
  fresh.push_back("b");
  fresh.push_back("c");
  fresh.push_back("d");
  
  EXPECT_FALSE(internal::impl::NeedToBeReordered(old, fresh));
  
  // a b c d -> a b d
  fresh.clear();
  fresh.push_back("a");
  fresh.push_back("b");
  fresh.push_back("d");
  
  EXPECT_FALSE(internal::impl::NeedToBeReordered(old, fresh));
  
  // a b c d -> a
  fresh.clear();
  fresh.push_back("a");
  
  EXPECT_FALSE(internal::impl::NeedToBeReordered(old, fresh));
  
  // a b c d ->  a d b
  fresh.clear();
  fresh.push_back("a");
  fresh.push_back("d");
  fresh.push_back("b");
  
  EXPECT_TRUE(internal::impl::NeedToBeReordered(old, fresh));
  
  // a b c d ->  b a c
  fresh.clear();
  fresh.push_back("b");
  fresh.push_back("a");
  fresh.push_back("c");
  
  EXPECT_TRUE(internal::impl::NeedToBeReordered(old, fresh));
}

TEST(TestFavoriteStorePrivate, TestNeedToBeReorderedPlus)
{
  std::list<std::string> old;
  std::list<std::string> fresh;
  
  old.push_back("a");
  old.push_back("b");
  old.push_back("c");
  old.push_back("d");
   
  // All new elements.
  fresh.clear();
  fresh.push_back("e");
  fresh.push_back("f");
  fresh.push_back("g");
  fresh.push_back("h");
  
  EXPECT_FALSE(internal::impl::NeedToBeReordered(old, fresh));
  
  // a b c d -> a b c d e
  fresh.clear();
  fresh.push_back("a");
  fresh.push_back("b");
  fresh.push_back("c");
  fresh.push_back("d");
  fresh.push_back("e");
  
  EXPECT_FALSE(internal::impl::NeedToBeReordered(old, fresh));
  
  // a b c d -> a b e c d
  fresh.clear();
  fresh.push_back("a");
  fresh.push_back("b");
  fresh.push_back("e");
  fresh.push_back("c");
  fresh.push_back("d");
  
  EXPECT_FALSE(internal::impl::NeedToBeReordered(old, fresh));
  
  // a b c d -> a b e d c
  fresh.clear();
  fresh.push_back("a");
  fresh.push_back("b");
  fresh.push_back("e");
  fresh.push_back("d");
  fresh.push_back("c");
  
  EXPECT_TRUE(internal::impl::NeedToBeReordered(old, fresh));
  
  // a b c d -> f a b c d
  fresh.clear();
  fresh.push_back("f");
  fresh.push_back("a");
  fresh.push_back("b");
  fresh.push_back("c");
  fresh.push_back("d");
  
  EXPECT_FALSE(internal::impl::NeedToBeReordered(old, fresh));
}

TEST(TestFavoriteStorePrivate, TestNeedToBeReorderedMixed)
{
  std::list<std::string> old;
  std::list<std::string> fresh;
  
  old.push_back("a");
  old.push_back("b");
  old.push_back("c");
  old.push_back("d");
  
  // a b c d -> b f c g h
  fresh.clear();
  fresh.push_back("b");
  fresh.push_back("f");
  fresh.push_back("c");
  fresh.push_back("g");
  fresh.push_back("h");
  
  EXPECT_FALSE(internal::impl::NeedToBeReordered(old, fresh));
  
  
  // a b c d -> c f b g h
  fresh.clear();
  fresh.push_back("c");
  fresh.push_back("f");
  fresh.push_back("b");
  fresh.push_back("g");
  fresh.push_back("h");
  
  EXPECT_TRUE(internal::impl::NeedToBeReordered(old, fresh));
}
   
