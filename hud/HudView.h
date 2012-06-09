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

#ifndef UNITYSHELL_HUD_VIEW_H
#define UNITYSHELL_HUD_VIEW_H

#include <string>

#include <Nux/Nux.h>
#include <Nux/VLayout.h>

#include "HudIcon.h"
#include "HudButton.h"
#include "HudAbstractView.h"
#include "unity-shared/SearchBar.h"
#include "unity-shared/OverlayRenderer.h"
#include "unity-shared/UBusWrapper.h"

namespace unity
{
namespace hud
{

class View : public AbstractView
{
  NUX_DECLARE_OBJECT_TYPE(View, AbstractView);
public:
  typedef nux::ObjectPtr<View> Ptr;

  View();
  ~View();

  void ResetToDefault();

  void Relayout();
  nux::View* default_focus() const;
  std::list<HudButton::Ptr> const& buttons() const;

  void SetQueries(Hud::Queries queries);
  void SetIcon(std::string const& icon_name, unsigned int tile_size, unsigned int size, unsigned int padding);
  void ShowEmbeddedIcon(bool show);
  void SearchFinished();

  void AboutToShow();
  void AboutToHide();

  void SetWindowGeometry(nux::Geometry const& absolute_geo, nux::Geometry const& geo);
  
protected:
  virtual Area* FindKeyFocusArea(unsigned int event_type,
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
  void OnSearchbarActivated();
  bool AcceptKeyNavFocus();
  nux::Geometry GetBestFitGeometry(nux::Geometry const& for_geo);

  void ProcessGrowShrink();

  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  UBusManager ubus;
  nux::ObjectPtr<nux::Layout> layout_;
  nux::ObjectPtr<nux::Layout> content_layout_;
  nux::ObjectPtr<nux::VLayout> button_views_;
  std::list<HudButton::Ptr> buttons_;

  //FIXME - replace with dash search bar once modifications to dash search bar land
  SearchBar::Ptr search_bar_;
  Icon::Ptr icon_;
  bool visible_;

  Hud::Queries queries_;
  nux::Geometry content_geo_;
  OverlayRenderer renderer_;
  nux::Geometry window_geometry_;
  nux::Geometry absolute_window_geometry_;

  guint timeline_id_;
  guint64 start_time_;
  int last_known_height_;
  int current_height_;
  bool timeline_need_more_draw_;
  int selected_button_;
  bool show_embedded_icon_;
  bool activated_signal_sent_;
};


} // namespace hud
} // namespace unity

#endif // UNITYSHELL_HUD_VIEW_H
