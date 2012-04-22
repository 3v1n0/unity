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


class Style
{
public:
  Style ();
  ~Style ();

  static Style& Instance();

  virtual bool Button(cairo_t* cr, nux::ButtonVisualState state,
                      std::string const& label, int font_size=-1,
                      Alignment alignment = Alignment::CENTER,
                      bool zeromargin=false);

  virtual bool SquareButton(cairo_t* cr, nux::ButtonVisualState state,
                            std::string const& label, bool curve_bottom,
                            int font_size=-1,
                            Alignment alignment = Alignment::CENTER,
                            bool zeromargin=false);

  virtual nux::AbstractPaintLayer* FocusOverlay(int width, int height);

  virtual bool ButtonFocusOverlay(cairo_t* cr);

  virtual bool MultiRangeSegment(cairo_t*    cr,
                                 nux::ButtonVisualState  state,
                                 std::string const& label,
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

  virtual int GetButtonGarnishSize();

  virtual int GetSeparatorGarnishSize();

  virtual int GetScrollbarGarnishSize();

  void Blur(cairo_t* cr, int size);

  void RoundedRect(cairo_t* cr,
                   double   aspect,
                   double   x,
                   double   y,
                   double   cornerRadius,
                   double   width,
                   double   height);

  nux::Color const& GetTextColor() const;

  // TODO nux::Property<int>
  int  GetDefaultNColumns() const;
  void SetDefaultNColumns(int n_cols);
  sigc::signal<void> columns_changed;

  int GetTileIconSize() const;
  int GetTileWidth() const;
  int GetTileHeight() const;

  int GetHomeTileIconSize() const;
  int GetHomeTileWidth() const;
  int GetHomeTileHeight() const;

  int GetTextLineHeight() const;

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

  int GetDashBottomTileHeight() const;
  int GetDashRightTileWidth() const;

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

  // Returns the width of the separator between the dash and the launcher.
  int GetVSeparatorSize() const;

  // Returns the height of the separator between the dash and the top panel.
  int GetHSeparatorSize() const;

  // Practically it is the space between the top border of the dash and the searchbar.
  int GetDashViewTopPadding() const;

  // Search bar
  int GetSearchBarLeftPadding() const;
  int GetSearchBarRightPadding() const;
  int GetSearchBarHeight() const;
  int GetFilterResultsHighlightRightPadding() const;
  int GetFilterResultsHighlightLeftPadding() const;

  // Filter bar
  int GetFilterBarTopPadding() const;
  int GetFilterHighlightPadding() const;
  int GetSpaceBetweenFilterWidgets() const;
  int GetAllButtonHeight() const;
  int GetFilterBarLeftPadding() const;
  int GetFilterBarRightPadding() const;
  int GetFilterBarWidth() const;
  int GetFilterButtonHeight() const;
  int GetFilterViewRightPadding() const;

  int GetSpaceBetweenLensAndFilters() const;

  // Scrollbars
  int GetScrollbarWidth() const;

  // Places Group
  int GetCategoryHighlightHeight() const;
  int GetPlacesGroupTopSpace() const;
  int GetCategoryHeaderLeftPadding() const;
  int GetCategorySeparatorLeftPadding() const;
  int GetCategorySeparatorRightPadding() const;

  sigc::signal<void> changed;

private:
  class Impl;
  Impl* pimpl;
};

}
}

#endif // DASH_STYLE_H
