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

#ifndef UNITY_TEXTURECACHE_H
#define UNITY_TEXTURECACHE_H

#include <string>
#include <map>

#include <Nux/Nux.h>
#include <sigc++/trackable.h>

/* A simple texture cache system, you ask the cache for a texture by id if the
 * texture does not exist it calls the callback function you provide it with
 * to create the texture, then returns it.  you should remember to ref/unref
 * the textures yourself however
 */
namespace unity
{

class TextureCache : public sigc::trackable
{
public:
  typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;

  ~TextureCache() = default;

  // id, width, height -> texture
  typedef std::function<nux::BaseTexture*(std::string const&, int, int)> CreateTextureCallback;

  static TextureCache& GetDefault();
  static nux::BaseTexture* DefaultTexturesLoader(std::string const&, int, int);

  BaseTexturePtr FindTexture(std::string const& texture_id, int width = 0, int height = 0, CreateTextureCallback callback = DefaultTexturesLoader);

  // Return the current size of the cache.
  std::size_t Size() const;

private:
  TextureCache() = default;

  std::string Hash(std::string const& , int width, int height);
  // Have the key passed by value not referece, as the key will be a bound
  // parameter in the slot passed to the texture on_destroy signal.
  void OnDestroyNotify(nux::Trackable* Object, std::string const& key);

  std::map<std::string, nux::BaseTexture*> cache_;
};

}

#endif // TEXTURECACHE_H
