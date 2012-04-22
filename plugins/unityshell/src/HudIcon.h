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
 * Authored by: Gord Allott <gord.allott@canonical.com>
 *
 */

#ifndef HUDICON_H
#define HUDICON_H

#include "IconTexture.h"
#include "HudIconTextureSource.h"
#include "IconRenderer.h"
#include "Introspectable.h"

namespace unity
{
namespace hud
{

class Icon : public unity::IconTexture
{
public:
  typedef nux::ObjectPtr<Icon> Ptr;
  Icon();

  void SetIcon(std::string const& icon_name, unsigned int icon_size, unsigned int tile_size);

protected:
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);

  std::string GetName() const;

private:
  nux::ObjectPtr<nux::BaseTexture> background_;
  nux::ObjectPtr<nux::BaseTexture> gloss_;
  nux::ObjectPtr<nux::BaseTexture> edge_;
  nux::ObjectPtr<HudIconTextureSource> icon_texture_source_;
  unity::ui::IconRenderer icon_renderer_;
};

}

}

#endif /* HUDICON_H */
