// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#ifndef ACTIONBUTTON_H
#define ACTIONBUTTON_H

#include <Nux/Nux.h>
#include <Nux/CairoWrapper.h>
#include <Nux/AbstractButton.h>

namespace nux
{
class StaticCairoText;
}

namespace unity
{
class IconTexture;

namespace dash
{

class ActionButton : public nux::AbstractButton
{
public:
  ActionButton(std::string const& label, std::string const& icon_hint, NUX_FILE_LINE_PROTO);
  ~ActionButton();

  sigc::signal<void, ActionButton*> click;

  void SetFont(std::string const& font_hint);

  void Activate();
  void Deactivate();

protected:
  virtual long ComputeContentSize();
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {}
  virtual void RecvClick(int x, int y, unsigned long button_flags, unsigned long key_flags);

  void Init();
  void InitTheme();
  void RedrawTheme(nux::Geometry const& geom, cairo_t* cr, nux::ButtonVisualState faked_state);
  void RedrawFocusOverlay(nux::Geometry const& geom, cairo_t* cr);

  void BuildLayout(std::string const& label, std::string const& icon_hint);


  typedef std::unique_ptr<nux::CairoWrapper> NuxCairoPtr;

  NuxCairoPtr cr_prelight_;
  NuxCairoPtr cr_active_;
  NuxCairoPtr cr_normal_;
  NuxCairoPtr cr_focus_;

  nux::Geometry cached_geometry_;

private:
  std::string icon_hint_;
  std::string font_hint_;

  nux::ObjectPtr<IconTexture> image_;
  nux::ObjectPtr<nux::StaticCairoText> static_text_;
};

} // namespace dash
} // namespace unity

#endif // ACTIONBUTTON_H
