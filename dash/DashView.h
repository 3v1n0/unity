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

#include <Nux/Nux.h>
#include <Nux/PaintLayer.h>
#include <Nux/View.h>
#include <Nux/VLayout.h>

#include <UnityCore/FilesystemLenses.h>
#include <UnityCore/HomeLens.h>
#include <UnityCore/GLibSource.h>

#include "ApplicationStarter.h"
#include "LensBar.h"
#include "LensView.h"
#include "previews/PreviewContainer.h"
#include "PreviewStateMachine.h"
#include "UnityCore/Preview.h"

#include "unity-shared/BackgroundEffectHelper.h"
#include "unity-shared/BGHash.h"
#include "unity-shared/Introspectable.h"
#include "unity-shared/OverlayRenderer.h"
#include "unity-shared/SearchBar.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/OverlayWindowButtons.h"


namespace na = nux::animation;

namespace unity
{
namespace dash
{

class DashLayout;

class DashView : public nux::View, public unity::debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(DashView, nux::View);
  typedef std::map<std::string, nux::ObjectPtr<LensView>> LensViews;

public:
   DashView(Lenses::Ptr const& lenses, ApplicationStarter::Ptr const& application_starter);
  ~DashView();

  void AboutToShow();
  void AboutToHide();
  void Relayout();
  void DisableBlur();
  void OnActivateRequest(GVariant* args);
  void SetMonitorOffset(int x, int y);

  bool IsCommandLensOpen() const;

  std::string const GetIdForShortcutActivation(std::string const& shortcut) const;
  std::vector<char> GetAllShortcuts();

  nux::View* default_focus() const;

  nux::Geometry const& GetContentGeometry() const;

protected:
  void ProcessDndEnter();

  virtual Area* FindKeyFocusArea(unsigned int key_symbol,
    unsigned long x11_key_code,
    unsigned long special_keys_state);

private:
  void SetupViews();
  void SetupUBusConnections();
  void OnBGColorChanged(GVariant *data);
  nux::Geometry GetBestFitGeometry(nux::Geometry const& for_geo);

  void Draw(nux::GraphicsEngine& gfx_context, bool force_draw);
  void DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw);
  virtual long PostLayoutManagement (long LayoutResult);
  nux::Area* FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type);

  // Dash animations
  void DrawDashSplit(nux::GraphicsEngine& graphics_engine, nux::Geometry& split_clip);
  void DrawPreviewContainer(nux::GraphicsEngine& graphics_engine);
  void DrawPreviewResultTextures(nux::GraphicsEngine& gfx_context, bool force_draw);
  void DrawPreview(nux::GraphicsEngine& gfx_context, bool force_draw);
  void StartPreviewAnimation();
  void EndPreviewAnimation();

  void BuildPreview(Preview::Ptr model);
  void ClosePreview();
  void OnPreviewAnimationFinished();
  void OnMouseButtonDown(int x, int y, unsigned long button, unsigned long key);
  void OnBackgroundColorChanged(GVariant* args);
  void OnSearchChanged(std::string const& search_string);
  void OnLiveSearchReached(std::string const& search_string);
  void OnLensAdded(Lens::Ptr& lens);
  void OnLensBarActivated(std::string const& id);
  void OnSearchFinished(Lens::Hints const& hints, glib::Error const& error);
  void OnGlobalSearchFinished(Lens::Hints const& hints, glib::Error const& error);
  void OnAppsGlobalSearchFinished(Lens::Ptr const& lens);
  void OnUriActivated(ResultView::ActivateType type, std::string const& uri, GVariant* data, std::string const& unique_id);
  void OnUriActivatedReply(std::string const& uri, HandledType type, Lens::Hints const&);
  bool DoFallbackActivation(std::string const& uri);
  bool LaunchApp(std::string const& appname);
  void OnEntryActivated();
  std::string AnalyseLensURI(std::string const& uri);
  void UpdateLensFilter(std::string lens, std::string filter, std::string value);
  void UpdateLensFilterValue(Filter::Ptr filter, std::string value);
  void EnsureLensesInitialized();

  bool AcceptKeyNavFocus();
  bool InspectKeyEvent(unsigned int eventType, unsigned int key_sym, const char* character);
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  nux::Area* KeyNavIteration(nux::KeyNavDirection direction);

  UBusManager ubus_manager_;
  Lenses::Ptr lenses_;
  HomeLens::Ptr home_lens_;
  LensViews lens_views_;

  ApplicationStarter::Ptr application_starter_;

  // View related
  PreviewStateMachine preview_state_machine_;
  previews::PreviewContainer::Ptr preview_container_;
  bool preview_displaying_;
  std::string stored_activated_unique_id_;
  dash::previews::Navigation preview_navigation_mode_;

  nux::VLayout* layout_;
  DashLayout* content_layout_;
  nux::View* content_view_;
  nux::HLayout* search_bar_layout_;
  SearchBar* search_bar_;
  nux::VLayout* lenses_layout_;
  LensBar* lens_bar_;

  nux::ObjectPtr<LensView> home_view_;
  nux::ObjectPtr<LensView> active_lens_view_;
  nux::ObjectPtr<LensView> preview_lens_view_;

  // Drawing related
  nux::Geometry content_geo_;
  OverlayRenderer renderer_;

  std::string last_activated_uri_;
  guint64 last_activated_timestamp_;
  bool search_in_progress_;
  bool activate_on_finish_;

  bool visible_;

  glib::Source::UniquePtr searching_timeout_;
  glib::Source::UniquePtr hide_message_delay_;

  nux::ObjectPtr<nux::IOpenGLBaseTexture> dash_view_copy_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> search_view_copy_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> filter_view_copy_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> layout_copy_;

  int opening_column_x_;
  int opening_row_y_;
  int opening_column_width_;
  int opening_row_height_;

  nux::Color background_color_;

  std::unique_ptr<na::AnimateValue<float>> split_animation_;
  float animate_split_value_;

  std::unique_ptr<na::AnimateValue<float>> preview_container_animation_;
  float animate_preview_container_value_;

  std::unique_ptr<na::AnimateValue<float>> preview_animation_;
  float animate_preview_value_;

  nux::ObjectPtr<OverlayWindowButtons> overlay_window_buttons_;
};


}
}
#endif
