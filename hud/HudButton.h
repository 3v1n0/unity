/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */

#ifndef UNITYSHELL_HUDBUTTON_H
#define UNITYSHELL_HUDBUTTON_H

#include <Nux/Nux.h>
#include <Nux/CairoWrapper.h>
#include <Nux/Button.h>
#include <Nux/TextureArea.h>
#include <UnityCore/Hud.h>
#include "unity-shared/Introspectable.h"

namespace nux { class HLayout; }

namespace unity
{
namespace hud
{

class HudButton : public nux::Button, public unity::debug::Introspectable 
{
  NUX_DECLARE_OBJECT_TYPE(HudButton, nux::Button);

public:
  typedef nux::ObjectPtr<HudButton> Ptr;

  HudButton(NUX_FILE_LINE_PROTO);

  void SetQuery(Query::Ptr query);
  std::shared_ptr<Query> GetQuery();

  void SetSkipDraw(bool skip_draw);

  nux::Property<std::string> label;
  nux::Property<bool> is_rounded;
  nux::Property<bool> fake_focused;

protected:
  virtual bool AcceptKeyNavFocus();
  virtual long ComputeContentSize();
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

  void InitTheme();
  void RedrawTheme(nux::Geometry const& geom, cairo_t* cr, nux::ButtonVisualState faked_state);

private:
  typedef std::unique_ptr<nux::CairoWrapper> NuxCairoPtr;

  Query::Ptr query_;
  nux::Geometry cached_geometry_;

  bool is_focused_;
  bool skip_draw_;

  NuxCairoPtr prelight_;
  NuxCairoPtr active_;
  NuxCairoPtr normal_;

  nux::HLayout* hlayout_;
};

} // namespace hud
} // namespace unity

#endif // UNITYSHELL_HUDBUTTON_H
