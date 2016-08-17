// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2016 Canonical Ltd
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
 *             Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "config.h"
#include "TextureCache.h"

#include "unity-shared/ThemeSettings.h"

namespace unity
{
namespace
{
// Stolen from boost
template <class T>
inline std::size_t hash_combine(std::size_t seed, T const& v)
{
  return seed ^ (std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

inline std::size_t Hash(std::string const& id, int width, int height)
{
  return hash_combine(hash_combine(std::hash<std::string>()(id), width), height);
}

inline nux::BaseTexture* create_2d_texture(std::string const& name, int w, int h)
{
  int size = std::max(w, h);
  return nux::CreateTexture2DFromFile(name.c_str(), (size <= 0 ? -1 : size), true);
}
}

TextureCache& TextureCache::GetDefault()
{
  static TextureCache instance;
  return instance;
}

TextureCache::TextureCache()
{
  theme::Settings::Get()->theme.changed.connect(sigc::mem_fun(this, &TextureCache::OnThemeChanged));
}

nux::BaseTexture* TextureCache::LocalLoader(std::string const& name, int w, int h)
{
  return create_2d_texture(PKGDATADIR"/" + name, w, h);
}

nux::BaseTexture* TextureCache::ThemedLoader(std::string const& name, int w, int h)
{
  auto& cache = GetDefault();
  cache.themed_files_.push_back(Hash(name, w, h));
  auto const& themed_file = theme::Settings::Get()->ThemedFilePath(name, {PKGDATADIR}, {""});
  return themed_file.empty() ? LocalLoader(name, w, h) : create_2d_texture(themed_file, w, h);
}

void TextureCache::OnThemeChanged(std::string const&)
{
  for (auto texture_key : themed_files_)
    cache_.erase(texture_key);

  themed_files_.clear();
  themed_invalidated.emit();
}

TextureCache::BaseTexturePtr TextureCache::FindTexture(std::string const& texture_id,
                                                       int width, int height,
                                                       CreateTextureCallback factory)
{
  if (!factory)
    return BaseTexturePtr();

  auto key = Hash(texture_id, width, height);
  auto texture_it = cache_.find(key);

  BaseTexturePtr texture(texture_it != cache_.end() ? texture_it->second : nullptr);

  if (!texture)
  {
    texture.Adopt(factory(texture_id, width, height));

    if (!texture)
      return texture;

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

    cache_.insert({key, texture.GetPointer()});
    auto on_destroy = sigc::mem_fun(this, &TextureCache::OnDestroyNotify);
    texture->OnDestroyed.connect(sigc::bind(on_destroy, key));
  }

  return texture;
}

void TextureCache::Invalidate(std::string const& texture_id, int width, int height)
{
  cache_.erase(Hash(texture_id, width, height));
}

void TextureCache::OnDestroyNotify(nux::Trackable* Object, std::size_t key)
{
  cache_.erase(key);
}

// Return the current size of the cache.
std::size_t TextureCache::Size() const
{
  return cache_.size();
}

} // namespace unity
