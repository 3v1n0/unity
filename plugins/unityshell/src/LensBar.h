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

#ifndef UNITY_LENS_BAR_H_
#define UNITY_LENS_BAR_H_

#include <string>
#include <vector>

#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/View.h>
#include <UnityCore/Lens.h>

#include "IconTexture.h"
#include "Introspectable.h"
#include "LensBarIcon.h"

namespace unity
{
namespace dash
{

class LensBar : public nux::View, public unity::debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(LensBar, nux::View);
  typedef std::vector<LensBarIcon*> LensIcons;

public:
  LensBar();
  ~LensBar();

  void AddLens(Lens::Ptr& lens);
  void Activate(std::string id);
  void ActivateNext();
  void ActivatePrevious();


  sigc::signal<void, std::string const&> lens_activated;

private:
  void SetupBackground();
  void SetupLayout();
  void SetupHomeLens();

  void Draw(nux::GraphicsEngine& gfx_context, bool force_draw);
  void DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw);

  void SetActive(LensBarIcon* icon);

  bool AcceptKeyNavFocus();
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  LensIcons icons_;

  nux::ColorLayer* bg_layer_;
  nux::HLayout* layout_;
};


}
}
#endif
