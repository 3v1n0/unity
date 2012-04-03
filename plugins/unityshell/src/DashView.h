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
#include <UnityCore/HomeLens.h>

#include "BackgroundEffectHelper.h"
#include "SearchBar.h"
#include "Introspectable.h"
#include "LensBar.h"
#include "LensView.h"
#include "UBusWrapper.h"
#include "OverlayRenderer.h"

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

  std::string const GetIdForShortcutActivation(std::string const& shortcut) const;
  std::vector<char> GetAllShortcuts();

  nux::View* default_focus() const;

protected:
  void ProcessDndEnter();

  virtual Area* FindKeyFocusArea(unsigned int key_symbol,
    unsigned long x11_key_code,
    unsigned long special_keys_state);

private:
  void SetupViews();
  void SetupUBusConnections();

  nux::Geometry GetBestFitGeometry(nux::Geometry const& for_geo);

  void Draw(nux::GraphicsEngine& gfx_context, bool force_draw);
  void DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw);
  virtual long PostLayoutManagement (long LayoutResult);

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

  static gboolean ResetSearchStateCb(gpointer data);
  static gboolean HideResultMessageCb(gpointer data);

private:
  UBusManager ubus_manager_;
  FilesystemLenses lenses_;
  HomeLens::Ptr home_lens_;
  LensViews lens_views_;


  // View related
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
  guint searching_timeout_id_;
  bool search_in_progress_;
  bool activate_on_finish_;

  guint hide_message_delay_id_;

  bool visible_;
};


}
}
#endif
