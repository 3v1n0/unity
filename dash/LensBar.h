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

#ifndef UNITYSHELL_LENS_BAR_H
#define UNITYSHELL_LENS_BAR_H

#include <memory>
#include <string>
#include <vector>

#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/View.h>
#include <UnityCore/Lens.h>

#include "unity-shared/IconTexture.h"
#include "unity-shared/Introspectable.h"
#include "LensBarIcon.h"

namespace nux
{
class AbstractPaintLayer;
}

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
  std::string GetActiveLensId() const;
  typedef std::unique_ptr<nux::AbstractPaintLayer> LayerPtr;

  LensIcons icons_;

  nux::HLayout* layout_;
  LayerPtr bg_layer_;
};

} // namespace dash
} // namespace unity

#endif // UNITYSHELL_LENS_BAR_H
