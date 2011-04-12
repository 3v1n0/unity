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

class PlacesStyle : public nux::Object
{
public:

  static PlacesStyle * GetDefault ();

  PlacesStyle ();
  ~PlacesStyle ();

  nux::Color& GetTextColor ();

  int  GetDefaultNColumns ();
  void SetDefaultNColumns (int n_cols);

  int GetTileIconSize ();
  int GetTileWidth    ();
  int GetTileHeight   ();

  int GetHomeTileIconSize ();
  int GetHomeTileWidth ();
  int GetHomeTileHeight ();

  nux::BaseTexture * GetDashBottomTile ();
  nux::BaseTexture * GetDashRightTile ();
  nux::BaseTexture * GetDashCorner ();
  nux::BaseTexture * GetDashFullscreenIcon ();

  nux::BaseTexture * GetSearchMagnifyIcon ();
  nux::BaseTexture * GetSearchCloseIcon ();
  nux::BaseTexture * GetSearchCloseGlowIcon ();
  nux::BaseTexture * GetSearchSpinIcon ();
  nux::BaseTexture * GetSearchSpinGlowIcon ();

  nux::BaseTexture * GetGroupUnexpandIcon ();
  nux::BaseTexture * GetGroupExpandIcon ();

  sigc::signal<void> changed;
  sigc::signal<void> columns_changed;

private:
  void               Refresh ();
  nux::BaseTexture * TextureFromFilename (const char *filename);

  static void OnFontChanged (GObject *object, GParamSpec *pspec, PlacesStyle *self);

private:
  nux::CairoGraphics _util_cg;
  nux::Color _text_color;

  int _text_width;
  int _text_height;
  int _n_cols;

  nux::BaseTexture *_dash_bottom_texture;
  nux::BaseTexture *_dash_right_texture;
  nux::BaseTexture *_dash_corner_texture;
  nux::BaseTexture *_dash_fullscreen_icon;

  nux::BaseTexture *_search_magnify_texture;
  nux::BaseTexture *_search_close_texture;
  nux::BaseTexture *_search_close_glow_texture;
  nux::BaseTexture *_search_spin_texture;
  nux::BaseTexture *_search_spin_glow_texture;

  nux::BaseTexture *_group_unexpand_texture;
  nux::BaseTexture *_group_expand_texture;
};

#endif // PLACES_STYLE_H
