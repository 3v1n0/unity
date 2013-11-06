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

#ifndef UNITYSHELL_FILTERMULTIRANGEBUTTON_H
#define UNITYSHELL_FILTERMULTIRANGEBUTTON_H

#include <memory>

#include <Nux/Nux.h>
#include <Nux/CairoWrapper.h>
#include <Nux/ToggleButton.h>
#include <Nux/View.h>
#include <UnityCore/Filter.h>

namespace unity
{
namespace dash
{

enum class MultiRangeSide : unsigned int
{
  LEFT = 0,
  RIGHT,
  CENTER
};

enum class MultiRangeArrow : unsigned int
{
  LEFT = 0,
  RIGHT,
  BOTH,
  NONE
};

class FilterMultiRangeButton : public nux::ToggleButton
{
  NUX_DECLARE_OBJECT_TYPE(FilterMultiRangeButton, nux::ToggleButton);
public:
  FilterMultiRangeButton (NUX_FILE_LINE_PROTO);
  virtual ~FilterMultiRangeButton() {}

  void SetFilter(FilterOption::Ptr const& filter);
  FilterOption::Ptr GetFilter();

  void SetVisualSide(MultiRangeSide side); //0 = left, 1 = center, 2 = right - sucky api i know :(
  void SetHasArrow(MultiRangeArrow arrow); //0 = left, 1 = both, 2 = right, -1 = none

protected:
  virtual long ComputeContentSize();
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);

private:
  void InitTheme();
  void Init();

  void RedrawTheme(nux::Geometry const& geom,
                   cairo_t* cr,
                   nux::ButtonVisualState faked_state,
                   MultiRangeArrow faked_arrow,
                   MultiRangeSide faked_side);

  void RedrawFocusOverlay(nux::Geometry const& geom,
                          cairo_t* cr,
                          MultiRangeArrow faked_arrow,
                          MultiRangeSide faked_side);
 
  void OnActivated(nux::Area* area);
  void OnActiveChanged(bool value);

  FilterOption::Ptr filter_;

  typedef std::unique_ptr<nux::CairoWrapper> NuxCairoPtr;
  typedef std::pair<MultiRangeArrow, MultiRangeSide> MapKey;

  std::map<MapKey, NuxCairoPtr> active_;
  std::map<MapKey, NuxCairoPtr> focus_;
  std::map<MapKey, NuxCairoPtr> normal_;
  std::map<MapKey, NuxCairoPtr> prelight_;
  bool theme_init_;

  nux::Geometry cached_geometry_;
  MultiRangeArrow has_arrow_;
  MultiRangeSide side_;
};

} // namespace dash
} // namespace unity

#endif // UNITYSHELL_FILTERMULTIRANGEBUTTON_H
