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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 */

#ifndef UNITY_HUD_VIEW_H_
#define UNITY_HUD_VIEW_H_

#include <string>

#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/Nux.h>
#include <Nux/PaintLayer.h>
#include <Nux/View.h>
#include <Nux/VLayout.h>
#include <StaticCairoText.h>

#include <glib.h>

#include <UnityCore/Hud.h>

#include "UBusWrapper.h"
#include "HudIcon.h"
#include "HudSearchBar.h"
#include "OverlayRenderer.h"

namespace unity
{
namespace hud
{

class View : public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(HudView, nux::View);
public:
  View();
  ~View();

  void ResetToDefault();

  void Relayout();
  nux::View* default_focus() const;

  void SetQueries(Hud::Queries queries);
  void SetIcon(std::string icon_name);
  
  void AboutToShow();
  void AboutToHide();

  void SetWindowGeometry(nux::Geometry const& absolute_geo, nux::Geometry const& geo);

  sigc::signal<void, std::string> search_changed;
  sigc::signal<void, std::string> search_activated;
  sigc::signal<void, Query::Ptr> query_activated;
  sigc::signal<void, Query::Ptr> query_selected;
  
protected:
  virtual Area* FindKeyFocusArea(unsigned int key_symbol,
  unsigned long x11_key_code,
  unsigned long special_keys_state);

  void SetupViews();
  void OnSearchChanged(std::string const& search_string);
  virtual long PostLayoutManagement(long LayoutResult);
private:
  void OnMouseButtonDown(int x, int y, unsigned long button, unsigned long key);
  void OnKeyDown (unsigned long event_type, unsigned long event_keysym,
                                unsigned long event_state, const TCHAR* character,
                                unsigned short key_repeat_count);
  void Draw(nux::GraphicsEngine& gfx_context, bool force_draw);
  void DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw);
  bool InspectKeyEvent(unsigned int eventType, unsigned int key_sym, const char* character);
  bool AcceptKeyNavFocus();
  nux::Geometry GetBestFitGeometry(nux::Geometry const& for_geo);

  const gchar* GetName();
  void AddProperties(GVariantBuilder* builder);

private:
  UBusManager ubus;
  nux::Layout* layout_;
  nux::Layout* content_layout_;
  nux::VLayout* button_views_;
  unity::hud::SearchBar* search_bar_;
  nux::StaticCairoText* search_hint_;
  Icon* icon_;
  bool visible_;

  Hud::Queries queries_;
  nux::Geometry content_geo_;
  OverlayRenderer renderer_;
  nux::ColorLayer* bg_layer_;
  nux::Geometry window_geometry_;
  nux::Geometry absolute_window_geometry_;
};


}
}
#endif

