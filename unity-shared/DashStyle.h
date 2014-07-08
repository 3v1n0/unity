// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com
 */

#ifndef DASH_STYLE_H
#define DASH_STYLE_H

#include "DashStyleInterface.h"

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <Nux/AbstractButton.h>

#include <cairo.h>

namespace nux
{
class AbstractPaintLayer;
}

namespace unity
{
namespace dash
{

enum class StockIcon {
  CHECKMARK,
  CROSS,
  GRID_VIEW,
  FLOW_VIEW,
  STAR
};

enum class Alignment {
  LEFT,
  CENTER,
  RIGHT
};

enum class Orientation {
  UP,
  DOWN,
  LEFT,
  RIGHT
};

enum class BlendMode {
  NORMAL,
  MULTIPLY,
  SCREEN
};

enum class FontWeight {
  LIGHT,
  REGULAR,
  BOLD
};

enum class Segment {
  LEFT,
  MIDDLE,
  RIGHT
};

enum class Arrow {
  LEFT,
  RIGHT,
  BOTH,
  NONE
};


class Style : public StyleInterface
{
public:
  Style ();
  ~Style ();

  static Style& Instance();

  virtual bool Button(cairo_t* cr, nux::ButtonVisualState state,
                      std::string const& label, int font_px_size=-1,
                      Alignment alignment = Alignment::CENTER,
                      bool zeromargin=false);

  virtual bool SquareButton(cairo_t* cr, nux::ButtonVisualState state,
                            std::string const& label, bool curve_bottom,
                            int font_px_size=-1,
                            Alignment alignment = Alignment::CENTER,
                            bool zeromargin=false);

  virtual nux::AbstractPaintLayer* FocusOverlay(int width, int height);

  virtual bool ButtonFocusOverlay(cairo_t* cr, float alpha = 0.50f);

  virtual bool MultiRangeSegment(cairo_t*    cr,
                                 nux::ButtonVisualState  state,
                                 std::string const& label,
                                 int font_px_size,
                                 Arrow       arrow,
                                 Segment     segment);

  virtual bool MultiRangeFocusOverlay(cairo_t* cr,
                                      Arrow arrow,
                                      Segment segment);

  virtual bool TrackViewNumber(cairo_t*    cr,
                               nux::ButtonVisualState  state,
                               std::string const& trackNumber);

  virtual bool TrackViewPlay(cairo_t*   cr,
                             nux::ButtonVisualState state);

  virtual bool TrackViewPause(cairo_t*   cr,
                              nux::ButtonVisualState state);

  virtual bool TrackViewProgress(cairo_t* cr);

  virtual bool SeparatorVert(cairo_t* cr);

  virtual bool SeparatorHoriz(cairo_t* cr);

  RawPixel GetButtonGarnishSize() const;

  RawPixel GetSeparatorGarnishSize() const;

  RawPixel GetScrollbarGarnishSize() const;

  void Blur(cairo_t* cr, int size);

  void RoundedRect(cairo_t* cr,
                   double   aspect,
                   double   x,
                   double   y,
                   double   cornerRadius,
                   double   width,
                   double   height);

  nux::Color const& GetTextColor() const;

  RawPixel GetTileGIconSize() const;
  RawPixel GetTileImageSize() const;
  RawPixel GetTileWidth() const;
  RawPixel GetTileHeight() const;
  RawPixel GetTileIconHightlightHeight() const;
  RawPixel GetTileIconHightlightWidth() const;

  RawPixel GetHomeTileIconSize() const;
  RawPixel GetHomeTileWidth() const;
  RawPixel GetHomeTileHeight() const;

  RawPixel GetTextLineHeight() const;

  nux::BaseTexture* GetCategoryBackground();
  nux::BaseTexture* GetCategoryBackgroundNoFilters();
  nux::BaseTexture* GetDashBottomTile();
  nux::BaseTexture* GetDashBottomTileMask();
  nux::BaseTexture* GetDashRightTile();
  nux::BaseTexture* GetDashRightTileMask();
  nux::BaseTexture* GetDashCorner();
  nux::BaseTexture* GetDashCornerMask();
  nux::BaseTexture* GetDashFullscreenIcon();
  nux::BaseTexture* GetDashLeftEdge();
  nux::BaseTexture* GetDashLeftCorner();
  nux::BaseTexture* GetDashLeftCornerMask();
  nux::BaseTexture* GetDashLeftTile();
  nux::BaseTexture* GetDashTopCorner();
  nux::BaseTexture* GetDashTopCornerMask();
  nux::BaseTexture* GetDashTopTile();

  RawPixel GetDashBottomTileHeight() const;
  RawPixel GetDashRightTileWidth() const;

  nux::BaseTexture* GetDashShine();

  nux::BaseTexture* GetSearchMagnifyIcon();
  nux::BaseTexture* GetSearchCircleIcon();
  nux::BaseTexture* GetSearchCloseIcon();
  nux::BaseTexture* GetSearchSpinIcon();

  nux::BaseTexture* GetGroupUnexpandIcon();
  nux::BaseTexture* GetGroupExpandIcon();

  nux::BaseTexture* GetStarDeselectedIcon();
  nux::BaseTexture* GetStarSelectedIcon();
  nux::BaseTexture* GetStarHighlightIcon();

  nux::BaseTexture* GetInformationTexture();

  nux::BaseTexture* GetRefineTextureCorner();
  nux::BaseTexture* GetRefineTextureDash();

  // Returns the width of the separator between the dash and the launcher.
  RawPixel GetVSeparatorSize() const;

  // Returns the height of the separator between the dash and the top panel.
  RawPixel GetHSeparatorSize() const;

  // Practically it is the space between the top border of the dash and the searchbar.
  RawPixel GetDashViewTopPadding() const;


  // Search bar
  RawPixel GetSearchBarLeftPadding() const;
  RawPixel GetSearchBarRightPadding() const;
  RawPixel GetSearchBarHeight() const;

  // Filter bar
  RawPixel GetFilterResultsHighlightRightPadding() const;
  RawPixel GetFilterResultsHighlightLeftPadding() const;
  RawPixel GetFilterBarTopPadding() const;
  RawPixel GetFilterHighlightPadding() const;
  RawPixel GetSpaceBetweenFilterWidgets() const;
  RawPixel GetAllButtonHeight() const;
  RawPixel GetFilterBarLeftPadding() const;
  RawPixel GetFilterBarRightPadding() const;
  RawPixel GetFilterBarWidth() const;
  RawPixel GetFilterButtonHeight() const;
  RawPixel GetFilterViewRightPadding() const;

  RawPixel GetSpaceBetweenScopeAndFilters() const;

  // Scrollbars
  RawPixel GetScrollbarWidth() const;

  // Places Group
  RawPixel GetCategoryIconSize() const;
  RawPixel GetCategoryHighlightHeight() const;
  RawPixel GetPlacesGroupTopSpace() const;
  RawPixel GetPlacesGroupResultTopPadding() const;
  RawPixel GetPlacesGroupResultLeftPadding() const;
  RawPixel GetCategoryHeaderLeftPadding() const;
  RawPixel GetCategorySeparatorLeftPadding() const;
  RawPixel GetCategorySeparatorRightPadding() const;

  sigc::signal<void> changed;

  nux::Property<int> columns_number;
  nux::Property<bool> always_maximised;
  nux::Property<bool> preview_mode;

private:
  class Impl;
  Impl* pimpl;
};

}
}

#endif // DASH_STYLE_H
