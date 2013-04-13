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
 * Authored by: Tim Penhey <tim.penhey@canonical.com>
 */

#include <gmock/gmock.h>

#include "TextureCache.h"

using namespace testing;
using namespace unity;

namespace
{
typedef nux::BaseTexture* (*TextureCallBack)(std::string const&, int, int);

TEST(TestTextureCache, TestInitiallyEmpty)
{
  TextureCache& cache = TextureCache::GetDefault();

  EXPECT_THAT(cache.Size(), Eq(0));
}

struct TextureCallbackValues
{
  std::string id;
  int width;
  int height;

  TextureCallbackValues() : width(0), height(0) {}
  nux::BaseTexture* callback(std::string const& i, int w, int h) {
    id = i;
    width = w;
    height = h;
    return nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableTexture();
  }
};

TEST(TestTextureCache, TestCallsCreateTextureCallback)
{
  // Another lambda issue.  If the lambda takes a reference to any other
  // variables, it seems incapable of assigning the function to the
  // TextureCallback type.
  TextureCallbackValues values;

  TextureCache& cache = TextureCache::GetDefault();
  TextureCache::CreateTextureCallback callback(sigc::mem_fun(values, &TextureCallbackValues::callback));

  nux::ObjectPtr<nux::BaseTexture> texture = cache.FindTexture("foo", 5, 7, callback);

  EXPECT_THAT(cache.Size(), Eq(1));
  EXPECT_THAT(values.id, Eq("foo"));
  EXPECT_THAT(values.width, Eq(5));
  EXPECT_THAT(values.height, Eq(7));
  EXPECT_THAT(texture->GetReferenceCount(), Eq(1));
}

struct TextureCallbackCounter
{
  int count;

  TextureCallbackCounter() : count(0) {}
  nux::BaseTexture* callback(std::string const& i, int w, int h) {
    ++count;
    return nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableTexture();
  }
};

TEST(TestTextureCache, TestCallbackOnlyCalledOnce)
{
  TextureCallbackCounter counter;
  TextureCache::CreateTextureCallback callback(sigc::mem_fun(counter, &TextureCallbackCounter::callback));

  TextureCache& cache = TextureCache::GetDefault();

  nux::ObjectPtr<nux::BaseTexture> t1 = cache.FindTexture("foo", 5, 7, callback);
  nux::ObjectPtr<nux::BaseTexture> t2 = cache.FindTexture("foo", 5, 7, callback);

  EXPECT_THAT(cache.Size(), Eq(1));
  EXPECT_THAT(counter.count, Eq(1));
  EXPECT_TRUE(t1 == t2);
}

TEST(TestTextureCache, TestCacheRemovesDeletedObject)
{
  // Note for others, if just using the lambda function, the return value is
  // lost in the type deduction that sigc uses.  So we have the typedef
  // (TextureCallback) at the top which includes the return value.  The lambda
  // constructor is fine to assign into this as it knows, and the explicit
  // return type is good for sigc.
  TextureCallBack callback =
    [](std::string const& i, int w, int h) -> nux::BaseTexture*
    {
      return nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableTexture();
    };

  TextureCache& cache = TextureCache::GetDefault();

  nux::ObjectPtr<nux::BaseTexture> t1 = cache.FindTexture("foo", 5, 7, callback);

  // Now delete the only reference to the texture.
  t1 = nux::ObjectPtr<nux::BaseTexture>();

  EXPECT_THAT(cache.Size(), Eq(0));
}


}
