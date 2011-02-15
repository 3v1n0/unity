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
 * Authored by Gordon Allott <gord.allott@canonical.com>
 */

#include "TextureCache.h"

TextureCache::TextureCache()
{

}

TextureCache::~TextureCache()
{

}

TextureCache *
TextureCache::GetDefault ()
{
  static TextureCache *default_loader = NULL;
  
  if (G_UNLIKELY (!default_loader))
    default_loader = new TextureCache ();
  
  return default_loader;
}

char *
TextureCache::Hash (const char *id, int width, int height)
{
  return g_strdup_printf ("%s-%ix%i", id, width, height);
}

nux::BaseTexture *
TextureCache::FindTexture (const char *texture_id, int width, int height, TextureCacheCallback slot)
{
  nux::BaseTexture *texture = NULL;
  if (texture_id == NULL)
  {
    g_error ("Did not supply a texture id to TextureCache::FindTexture");
    return NULL;
  }

  char *key = Hash (texture_id, width, height);
  texture = _cache[key];

  if (texture == NULL)
  {
    // no texture yet, lets make one
    slot (texture_id, width, height, &texture);

    TexDestroyPayload *payload;
    
    payload = g_slice_new0 (TexDestroyPayload);
    payload->self = this;
    payload->texid = g_strdup (key);

    texture->add_destroy_notify_callback (payload, &(TextureCache::OnDestroyNotify));

    _cache[key] = texture;
  }

  g_free (key);

  return texture;
}

void *
TextureCache::OnDestroyNotify (void *data)
{
  TexDestroyPayload *payload = (TexDestroyPayload *)data;
  TextureCache *self = payload->self;

  self->_cache.erase (self->_cache.find (payload->texid));
  g_free (payload->texid);

  g_slice_free (TexDestroyPayload, payload);
  return 0;
}
