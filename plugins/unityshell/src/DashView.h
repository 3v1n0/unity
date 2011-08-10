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

#ifndef UNITY_DASH_VIEW_H_
#define UNITY_DASH_VIEW_H_

#include <string>

#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/Nux.h>
#include <Nux/PaintLayer.h>
#include <Nux/View.h>
#include <Nux/VLayout.h>
#include <UnityCore/FilesystemLenses.h>

#include "BackgroundEffectHelper.h"
#include "DashSearchBar.h"
#include "Introspectable.h"
#include "LensBar.h"
#include "LensView.h"
#include "UBusWrapper.h"

namespace unity
{
namespace dash
{

enum SizeMode
{
  SIZE_MODE_MAXIMISED,
  SIZE_MODE_NORMAL,
  SIZE_MODE_VERTICAL_MAXIMISED,
  SIZE_MODE_HORIZONATAL_MAXIMISED
};

class DashView : public nux::View, public unity::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(DashView, nux::View);
  typedef std::map<std::string, LensView*> LensViews;

public:
  DashView();
  ~DashView();

  void AboutToShow();
  void Relayout();

  nux::View* default_focus() const;

private:
  void SetupBackground();
  void SetupViews();
  void SetupUBusConnections();

  nux::Geometry GetBestFitGeometry(nux::Geometry const& for_geo);

  long ProcessEvent(nux::IEvent& ievent, long traverse_info, long event_info);
  void Draw(nux::GraphicsEngine& gfx_context, bool force_draw);
  void DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw);

  void OnMouseButtonDown(int x, int y, unsigned long button, unsigned long key);
  void OnActivateRequest(GVariant* args);
  void OnBackgroundColorChanged(GVariant* args);
  void OnSearchChanged(std::string const& search_string);
  void OnLiveSearchReached(std::string const& search_string);
  void OnLensAdded(Lens::Ptr& lens);
  void OnLensBarActivated(std::string const& id);
  void OnSearchFinished(std::string const& search_string);
  void OnUriActivated(std::string const& uri);
  void OnUriActivatedReply(std::string const& uri, HandledType type, Lens::Hints const&);
  bool DoFallbackActivation(std::string const& uri);
  bool LaunchApp(std::string const& appname);
  
  bool AcceptKeyNavFocus();
  bool InspectKeyEvent(unsigned int eventType, unsigned int key_sym, const char* character);
  const gchar* GetName();
  void AddProperties(GVariantBuilder* builder);

private:
  UBusManager ubus_manager_;
  FilesystemLenses lenses_;
  SizeMode size_mode_;
  BackgroundEffectHelper bg_effect_helper_;
  LensViews lens_views_;

  // Background related
  nux::ColorLayer* bg_layer_;

  // View related
  nux::VLayout* layout_;
  nux::VLayout* content_layout_;
  SearchBar* search_bar_;
  nux::VLayout* lenses_layout_;
  LensBar* lens_bar_;

  LensView* active_lens_view_;

  // Drawing related
  nux::Geometry content_geo_;
  nux::ObjectPtr <nux::IOpenGLBaseTexture> bg_blur_texture_;

  std::string last_activated_uri_;
};


}
}
#endif
