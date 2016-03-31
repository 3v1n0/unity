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
#include <unordered_map>

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
  static nux::BaseTexture* LocalLoader(std::string const&, int, int);
  static nux::BaseTexture* ThemedLoader(std::string const&, int, int);

  BaseTexturePtr FindTexture(std::string const& texture_id, int width = 0, int height = 0, CreateTextureCallback callback = ThemedLoader);
  void Invalidate(std::string const& texture_id, int width = 0, int height = 0);

  // Return the current size of the cache.
  std::size_t Size() const;

  sigc::signal<void> themed_invalidated;

private:
  TextureCache();

  void OnDestroyNotify(nux::Trackable* Object, std::size_t key);
  void OnThemeChanged(std::string const&);

  std::unordered_map<std::size_t, nux::BaseTexture*> cache_;
  std::vector<std::size_t> themed_files_;
};

}

#endif // TEXTURECACHE_H
