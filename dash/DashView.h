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

#include "unity-shared/BackgroundEffectHelper.h"
#include "unity-shared/SearchBar.h"
#include "unity-shared/Introspectable.h"
#include "unity-shared/BGHash.h"
#include "LensBar.h"
#include "LensView.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/OverlayRenderer.h"
#include "UnityCore/Preview.h"
#include "previews/PreviewContainer.h"
#include "PreviewStateMachine.h"

namespace na = nux::animation;

namespace unity
{
namespace dash
{

class DashLayout;

class DashView : public nux::View, public unity::debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(DashView, nux::View);
  typedef std::map<std::string, LensView*> LensViews;

public:
  DashView();
  ~DashView();

  void AboutToShow();
  void AboutToHide();
  void Relayout();
  void DisableBlur();
  void OnActivateRequest(GVariant* args);
  void SetMonitorOffset(int x, int y);

  void SetPreview(Preview::Ptr preview);
  void ClosePreview();

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
  
  void BuildPreview(Preview::Ptr model);
  void OnMouseButtonDown(int x, int y, unsigned long button, unsigned long key);
  void OnBackgroundColorChanged(GVariant* args);
  void OnSearchChanged(std::string const& search_string);
  void OnLiveSearchReached(std::string const& search_string);
  void OnLensAdded(Lens::Ptr& lens);
  void OnLensBarActivated(std::string const& id);
  void OnSearchFinished(Lens::Hints const& hints);
  void OnGlobalSearchFinished(Lens::Hints const& hints);
  void OnAppsGlobalSearchFinished(Lens::Hints const& hints);
  void OnUriActivated(std::string const& uri);
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
  FilesystemLenses lenses_;
  HomeLens::Ptr home_lens_;
  LensViews lens_views_;

  // View related
  PreviewStateMachine preview_state_machine_;
  previews::PreviewContainer::Ptr preview_container_;
  bool preview_displaying_;
  std::string stored_preview_unique_id_;
  std::string stored_preview_uri_identifier_;
  dash::previews::Navigation preview_navigation_mode_;

  nux::VLayout* layout_;
  DashLayout* content_layout_;
  nux::HLayout* search_bar_layout_;
  SearchBar* search_bar_;
  nux::VLayout* lenses_layout_;
  LensBar* lens_bar_;

  LensView* home_view_;
  LensView* active_lens_view_;

  // Drawing related
  nux::Geometry content_geo_;
  OverlayRenderer renderer_;

  std::string last_activated_uri_;
  // we're passing this back to g_* functions, so we'll keep the g* type
  bool search_in_progress_;
  bool activate_on_finish_;

  bool visible_;

  glib::Source::UniquePtr searching_timeout_;
  glib::Source::UniquePtr hide_message_delay_;

  nux::ObjectPtr<nux::IOpenGLBaseTexture> dash_view_copy_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> search_view_copy_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> filter_view_copy_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> layout_copy_;

  float fade_out_value_;
  float fade_in_value_;
  na::AnimateValue<float> animation_;

  void FadeOutCallBack(float const& fade_out_value);
  void FadeInCallBack(float const& fade_out_value);

  int opening_row_y_;
  int opening_row_height_;

  sigc::connection fade_in_connection_;
  sigc::connection fade_out_connection_;

  nux::Color background_color_;
};


}
}
#endif
