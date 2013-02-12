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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef UNITY_SCOPE_BAR_ICON_H_
#define UNITY_SCOPE_BAR_ICON_H_

#include <string>

#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/View.h>

#include "unity-shared/IconTexture.h"

namespace unity
{
namespace dash
{

class ScopeBarIcon : public IconTexture
{
  NUX_DECLARE_OBJECT_TYPE(ScopeBarIcon, IconTexture);
public:
  ScopeBarIcon(std::string id, std::string icon_hint);
  ~ScopeBarIcon();

  nux::Property<std::string> id;
  nux::Property<bool> active;

private:
  void Draw(nux::GraphicsEngine& gfx_context, bool force_draw);
  void OnActiveChanged(bool is_active);

  // Introspectable
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  typedef std::unique_ptr<nux::AbstractPaintLayer> LayerPtr;

  const float inactive_opacity_;
  LayerPtr focus_layer_;
};

}
}
#endif // UNITY_SCOPE_BAR_ICON_H_
