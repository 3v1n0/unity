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

#ifndef UNITYSHELL_SCOPE_BAR_H
#define UNITYSHELL_SCOPE_BAR_H

#include <memory>
#include <string>
#include <vector>

#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/Nux.h>
#include <Nux/PaintLayer.h>
#include <Nux/View.h>
#include <UnityCore/Scope.h>

#include "unity-shared/IconTexture.h"
#include "unity-shared/Introspectable.h"
#include "unity-shared/UBusWrapper.h"
#include "ScopeBarIcon.h"

namespace nux
{
class AbstractPaintLayer;
class HLayout;
class LayeredLayout;
}

namespace unity
{
class IconTexture;
class StaticCairoText;

namespace dash
{

class ScopeBar : public nux::View, public unity::debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(ScopeBar, nux::View);
  typedef std::vector<ScopeBarIcon*> ScopeIcons;

public:
  ScopeBar();

  void AddScope(Scope::Ptr const& scope);
  void Activate(std::string id);
  void ActivateNext();
  void ActivatePrevious();

  std::string GetActiveScopeId() const;

  sigc::signal<void, std::string const&> scope_activated;

private:
  void SetupBackground();
  void SetupLayout();
  void DoOpenLegalise();

  void Draw(nux::GraphicsEngine& gfx_context, bool force_draw);
  void DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw);

  nux::Area* FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type);

  void SetActive(ScopeBarIcon* icon);

  bool AcceptKeyNavFocus();
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  typedef std::unique_ptr<nux::AbstractPaintLayer> LayerPtr;

  ScopeIcons icons_;

  UBusManager ubus_;

  nux::LayeredLayout* layered_layout_;
  nux::HLayout *legal_layout_;
  unity::StaticCairoText *legal_;
  nux::HLayout* layout_;
  LayerPtr bg_layer_;
  IconTexture* info_icon_;

  bool info_previously_shown_;
  std::string legal_seen_file_path_;

  friend class TestScopeBar;
};

} // namespace dash
} // namespace unity

#endif // UNITYSHELL_SCOPE_BAR_H
