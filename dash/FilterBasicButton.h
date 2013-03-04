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

#ifndef UNITYSHELL_FILTERBASICBUTTON_H
#define UNITYSHELL_FILTERBASICBUTTON_H

#include <Nux/Nux.h>
#include <Nux/CairoWrapper.h>
#include <Nux/ToggleButton.h>

namespace unity
{
namespace dash
{

class FilterBasicButton : public nux::ToggleButton
{
  NUX_DECLARE_OBJECT_TYPE(FilterBasicButton, nux::ToggleButton);
public:
  FilterBasicButton(nux::TextureArea* image, NUX_FILE_LINE_PROTO);
  FilterBasicButton(std::string const& label, NUX_FILE_LINE_PROTO);
  FilterBasicButton(std::string const& label, nux::TextureArea* image, NUX_FILE_LINE_PROTO);
  FilterBasicButton(NUX_FILE_LINE_PROTO);
  virtual ~FilterBasicButton();

  void SetClearBeforeDraw(bool clear_before_draw);
protected:
  virtual long ComputeContentSize();
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);

  void Init();
  void InitTheme();
  void RedrawTheme(nux::Geometry const& geom, cairo_t* cr, nux::ButtonVisualState faked_state);
  void RedrawFocusOverlay(nux::Geometry const& geom, cairo_t* cr);

  typedef std::unique_ptr<nux::CairoWrapper> NuxCairoPtr;

  NuxCairoPtr prelight_;
  NuxCairoPtr active_;
  NuxCairoPtr normal_;
  NuxCairoPtr focus_;

  nux::Geometry cached_geometry_;

private:
  std::string label_;
  bool clear_before_draw_;
};

} // namespace dash
} // namespace unity

#endif // UNITYSHELL_FILTERBASICBUTTON_H
