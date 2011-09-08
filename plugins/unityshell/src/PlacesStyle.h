/*
 * Copyright (C) 2010 Canonical Ltd
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

#ifndef PLACES_STYLE_H
#define PLACES_STYLE_H

#include <Nux/Nux.h>
#include <NuxImage/CairoGraphics.h>
#include <gtk/gtk.h>

#include <UnityCore/GLibSignal.h>

namespace unity
{

class PlacesStyle
{
public:
  static PlacesStyle* GetDefault();

  PlacesStyle();
  ~PlacesStyle();

  nux::Color const& GetTextColor() const;

  int  GetDefaultNColumns() const;
  void SetDefaultNColumns(int n_cols);

  int GetTileIconSize() const;
  int GetTileWidth() const;
  int GetTileHeight() const;

  int GetHomeTileIconSize() const;
  int GetHomeTileWidth() const;
  int GetHomeTileHeight() const;

  int GetTextLineHeight() const;

  nux::BaseTexture* GetDashBottomTile();
  nux::BaseTexture* GetDashRightTile();
  nux::BaseTexture* GetDashCorner();
  nux::BaseTexture* GetDashFullscreenIcon();
  nux::BaseTexture* GetDashLeftEdge();
  nux::BaseTexture* GetDashLeftCorner();
  nux::BaseTexture* GetDashLeftTile();
  nux::BaseTexture* GetDashTopCorner();
  nux::BaseTexture* GetDashTopTile();

  nux::BaseTexture* GetSearchMagnifyIcon();
  nux::BaseTexture* GetSearchCloseIcon();
  nux::BaseTexture* GetSearchCloseGlowIcon();
  nux::BaseTexture* GetSearchSpinIcon();
  nux::BaseTexture* GetSearchSpinGlowIcon();

  nux::BaseTexture* GetGroupUnexpandIcon();
  nux::BaseTexture* GetGroupExpandIcon();

  sigc::signal<void> changed;
  sigc::signal<void> columns_changed;

private:
  void Refresh();
  void OnFontChanged(GtkSettings* object, GParamSpec* pspec);

  typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;
  BaseTexturePtr TextureFromFilename(const char* filename);

private:
  glib::SignalManager signal_manager_;

  nux::CairoGraphics _util_cg;
  nux::Color _text_color;

  int _text_width;
  int _text_height;
  int _n_cols;

  BaseTexturePtr _dash_bottom_texture;
  BaseTexturePtr _dash_right_texture;
  BaseTexturePtr _dash_corner_texture;
  BaseTexturePtr _dash_fullscreen_icon;
  BaseTexturePtr _dash_left_edge;
  BaseTexturePtr _dash_left_corner;
  BaseTexturePtr _dash_left_tile;
  BaseTexturePtr _dash_top_corner;
  BaseTexturePtr _dash_top_tile;

  BaseTexturePtr _search_magnify_texture;
  BaseTexturePtr _search_close_texture;
  BaseTexturePtr _search_close_glow_texture;
  BaseTexturePtr _search_spin_texture;
  BaseTexturePtr _search_spin_glow_texture;

  BaseTexturePtr _group_unexpand_texture;
  BaseTexturePtr _group_expand_texture;
};

}

#endif // PLACES_STYLE_H
