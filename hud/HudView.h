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
#include <UnityCore/GLibSource.h>

#include "HudIcon.h"
#include "HudButton.h"
#include "HudAbstractView.h"
#include "unity-shared/BackgroundEffectHelper.h"
#include "unity-shared/OverlayRenderer.h"
#include "unity-shared/OverlayWindowButtons.h"
#include "unity-shared/SearchBar.h"
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

  nux::View* default_focus() const;
  std::list<HudButton::Ptr> const& buttons() const;

  void SetQueries(Hud::Queries queries);
  void SetIcon(std::string const& icon_name, unsigned int tile_size, unsigned int size, unsigned int padding);
  void ShowEmbeddedIcon(bool show);
  void SearchFinished();

  void AboutToShow();
  void AboutToHide();

  void SetMonitorOffset(int x, int y);
  
  nux::Geometry GetContentGeometry();

protected:
  virtual Area* FindKeyFocusArea(unsigned int event_type,
  unsigned long x11_key_code,
  unsigned long special_keys_state);

  void SetupViews();
  void OnSearchChanged(std::string const& search_string);

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
  void UpdateLayoutGeometry();

  void ProcessGrowShrink();

  void MouseStealsHudButtonFocus();
  void LoseSelectedButtonFocus();
  void FindNewSelectedButton();
  void SelectLastFocusedButton();

  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);
  IntrospectableList GetIntrospectableChildren();

private:
  UBusManager ubus;
  nux::ObjectPtr<nux::Layout> layout_;
  nux::ObjectPtr<nux::Layout> content_layout_;
  nux::ObjectPtr<nux::VLayout> button_views_;
  std::list<HudButton::Ptr> buttons_;
  IntrospectableList introspectable_children_;

  //FIXME - replace with dash search bar once modifications to dash search bar land
  SearchBar::Ptr search_bar_;
  Icon::Ptr icon_;
  bool visible_;

  Hud::Queries queries_;
  nux::Geometry content_geo_;
  OverlayRenderer renderer_;
  glib::Source::UniquePtr timeline_idle_;
  bool timeline_animating_;

  guint64 start_time_;
  int last_known_height_;
  int current_height_;
  int selected_button_;
  bool show_embedded_icon_;
  bool activated_signal_sent_;
  bool keyboard_stole_focus_;

  nux::ObjectPtr<OverlayWindowButtons> overlay_window_buttons_;
};


} // namespace hud
} // namespace unity

#endif // UNITYSHELL_HUD_VIEW_H
