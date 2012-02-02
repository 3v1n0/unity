// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 */


#include "HudIconTextureSource.h"
#include "config.h"

#include <glib.h>

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>


namespace unity
{
namespace hud
{

HudIconTextureSource::HudIconTextureSource(nux::ObjectPtr<nux::BaseTexture> texture)
  : unity::ui::IconTextureSource()
  , icon_texture_(texture)
{
}

HudIconTextureSource::~HudIconTextureSource()
{
}

nux::Color HudIconTextureSource::BackgroundColor()
{
  return nux::Color(0.8f, 0.5f, 0.1f, 1.0f);
}

nux::BaseTexture* HudIconTextureSource::TextureForSize(int size)
{
  return icon_texture_.GetPointer();
}

nux::Color HudIconTextureSource::GlowColor()
{
 return nux::Color(0.0f, 0.0f, 0.0f, 0.0f); 
}

nux::BaseTexture* HudIconTextureSource::Emblem()
{
  return nullptr;
}

}
}