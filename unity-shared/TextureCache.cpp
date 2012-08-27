// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010, 2011 Canonical Ltd
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
 * Authored by Gordon Allott <gord.allott@canonical.com>
 *             Tim Penhey <tim.penhey@canonical.com>
 */

#include "TextureCache.h"

#include <sstream>
#include <NuxCore/Logger.h>

namespace unity
{
namespace
{
nux::logging::Logger logger("unity.internal.texturecache");
}

TextureCache::TextureCache()
{
}

TextureCache::~TextureCache()
{
}

TextureCache& TextureCache::GetDefault()
{
  static TextureCache instance;
  return instance;
}

std::string TextureCache::Hash(std::string const& id, int width, int height)
{
  std::ostringstream sout;
  sout << id << "-" << width << "x" << height;
  return sout.str();
}

TextureCache::BaseTexturePtr TextureCache::FindTexture(std::string const& texture_id,
                                                       int width, int height,
                                                       CreateTextureCallback slot)
{
  if (!slot)
    return BaseTexturePtr();

  std::string key = Hash(texture_id, width, height);
  BaseTexturePtr texture(cache_[key]);

  if (!texture)
  {
    texture = slot(texture_id, width, height);

    // Now here is the magic.
    //
    // The slot function is required to return a new texture.  This has an
    // internal reference count of one.  The TextureCache wants to hold the
    // texture inside the internal map for as long as the object itself
    // exists, but it doesn't want any ownership of the texture.  How we
    // handle this is to always return a smart pointer for the texture.  These
    // smart pointers have the ownership of the texture.
    //
    // We also hook into the objects OnDestroyed signal to remove it from the
    // map.  To avoid a reverse lookup map, we bind the key to the method
    // call.  By using a mem_fun rather than a lambda function, and through
    // the magic of sigc::trackable, we don't need to remember the connection
    // itself.  We get notified when the object is being destroyed, and if we
    // are destroyed first, then the sigc::trackable disconnects all methods
    // created using mem_fun.

    // Reduce the internal reference count of the texture, so the smart
    // pointer is the sole owner of the object.
    texture->UnReference();

    cache_[key] = texture.GetPointer();

    auto on_destroy = sigc::mem_fun(this, &TextureCache::OnDestroyNotify);
    texture->OnDestroyed.connect(sigc::bind(on_destroy, key));
  }

  return texture;
}

void TextureCache::OnDestroyNotify(nux::Trackable* Object, std::string key)
{
  cache_.erase(key);
}

// Return the current size of the cache.
std::size_t TextureCache::Size() const
{
  return cache_.size();
}

} // namespace unity
