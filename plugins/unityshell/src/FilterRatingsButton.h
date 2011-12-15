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

#ifndef UNITYSHELL_FILTERRATINGSBUTTONWIDGET_H
#define UNITYSHELL_FILTERRATINGSBUTTONWIDGET_H

#include <Nux/Nux.h>
#include <Nux/ToggleButton.h>
#include <Nux/CairoWrapper.h>
#include <UnityCore/RatingsFilter.h>

namespace unity
{
namespace dash
{

class FilterRatingsButton : public nux::ToggleButton
{
public:
  FilterRatingsButton(NUX_FILE_LINE_PROTO);
  virtual ~FilterRatingsButton();

  void SetFilter(dash::Filter::Ptr filter);
  dash::RatingsFilter::Ptr GetFilter();
  std::string GetFilterType();

protected:
  virtual long ComputeContentSize();
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

  void InitTheme();

  void RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  void OnRatingsChanged(int rating);

  nux::CairoWrapper* active_empty_;
  nux::CairoWrapper* normal_empty_;
  nux::CairoWrapper* active_half_;
  nux::CairoWrapper* normal_half_;
  nux::CairoWrapper* active_full_;
  nux::CairoWrapper* normal_full_;
  nux::Geometry cached_geometry_;

  void RedrawTheme(nux::Geometry const& geom, cairo_t* cr, int type, nux::ButtonVisualState faked_state);

  dash::RatingsFilter::Ptr filter_;

};

} // namespace dash
} // namespace unity

#endif // UNITYSHELL_FILTERRATINGSBUTTONWIDGET_H
