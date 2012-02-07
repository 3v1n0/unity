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

#include "DashView.h"
#include "DashViewPrivate.h"

#include <math.h>

#include <gio/gdesktopappinfo.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <NuxCore/Logger.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/RadioOptionFilter.h>

#include "DashStyle.h"
#include "DashSettings.h"
#include "UBusMessages.h"

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.view");
}

NUX_IMPLEMENT_OBJECT_TYPE(DashView);

DashView::DashView()
  : nux::View(NUX_TRACKER_LOCATION)
  , home_lens_(new HomeLens(_("Home"), _("Home screen"), _("Search")))
  , active_lens_view_(0)
  , last_activated_uri_("")
  , searching_timeout_id_(0)
  , search_in_progress_(false)
  , activate_on_finish_(false)
  , visible_(false)
{
  renderer_.SetOwner(this);
  renderer_.need_redraw.connect([this] () { 
    QueueDraw();
  });
  
  SetupViews();
  SetupUBusConnections();

  Settings::Instance().changed.connect(sigc::mem_fun(this, &DashView::Relayout));
  lenses_.lens_added.connect(sigc::mem_fun(this, &DashView::OnLensAdded));
  mouse_down.connect(sigc::mem_fun(this, &DashView::OnMouseButtonDown));

  Relayout();

  home_lens_->AddLenses(lenses_);
  lens_bar_->Activate("home.lens");
}

DashView::~DashView()
{
  if (searching_timeout_id_)
    g_source_remove (searching_timeout_id_);
}

void DashView::SetMonitorOffset(int x, int y)
{
  renderer_.x_offset = x;
  renderer_.y_offset = y;
}

void DashView::AboutToShow()
{
  ubus_manager_.SendMessage(UBUS_BACKGROUND_REQUEST_COLOUR_EMIT);
  visible_ = true;
  search_bar_->text_entry()->SelectAll();

  /* Give the lenses a chance to prep data before we map them  */
  lens_bar_->Activate(active_lens_view_->lens()->id());
  if (active_lens_view_->lens()->id() == "home.lens")
  {
    for (auto lens : lenses_.GetLenses())
      {
        lens->view_type = ViewType::HOME_VIEW;
        LOG_DEBUG(logger) << "Setting ViewType " << ViewType::HOME_VIEW
                              << " on '" << lens->id() << "'";
      }

      home_lens_->view_type = ViewType::LENS_VIEW;
      LOG_DEBUG(logger) << "Setting ViewType " << ViewType::LENS_VIEW
                                << " on '" << home_lens_->id() << "'";
  }

  renderer_.AboutToShow();
}

void DashView::AboutToHide()
{
  visible_ = false;
  renderer_.AboutToHide();

  for (auto lens : lenses_.GetLenses())
  {
    lens->view_type = ViewType::HIDDEN;
    LOG_DEBUG(logger) << "Setting ViewType " << ViewType::HIDDEN
                          << " on '" << lens->id() << "'";
  }

  home_lens_->view_type = ViewType::HIDDEN;
  LOG_DEBUG(logger) << "Setting ViewType " << ViewType::HIDDEN
                            << " on '" << home_lens_->id() << "'";
}

void DashView::SetupViews()
{
  layout_ = new nux::VLayout();
  SetLayout(layout_);

  content_layout_ = new nux::VLayout();
  content_layout_->SetHorizontalExternalMargin(0);
  content_layout_->SetVerticalExternalMargin(0);

  layout_->AddLayout(content_layout_, 1, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
  search_bar_ = new SearchBar();
  AddChild(search_bar_);
  search_bar_->activated.connect(sigc::mem_fun(this, &DashView::OnEntryActivated));
  search_bar_->search_changed.connect(sigc::mem_fun(this, &DashView::OnSearchChanged));
  search_bar_->live_search_reached.connect(sigc::mem_fun(this, &DashView::OnLiveSearchReached));
  search_bar_->showing_filters.changed.connect([&] (bool showing) { if (active_lens_view_) active_lens_view_->filters_expanded = showing; QueueDraw(); });
  content_layout_->AddView(search_bar_, 0, nux::MINOR_POSITION_LEFT);

  lenses_layout_ = new nux::VLayout();
  content_layout_->AddView(lenses_layout_, 1, nux::MINOR_POSITION_LEFT);

  home_view_ = new LensView(home_lens_);
  AddChild(home_view_);
  active_lens_view_ = home_view_;
  lens_views_[home_lens_->id] = home_view_;
  lenses_layout_->AddView(home_view_);

  lens_bar_ = new LensBar();
  AddChild(lens_bar_);
  lens_bar_->lens_activated.connect(sigc::mem_fun(this, &DashView::OnLensBarActivated));
  content_layout_->AddView(lens_bar_, 0, nux::MINOR_POSITION_CENTER);
}

void DashView::SetupUBusConnections()
{
  ubus_manager_.RegisterInterest(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
      sigc::mem_fun(this, &DashView::OnActivateRequest));
}

long DashView::PostLayoutManagement (long LayoutResult)
{
  Relayout();
  return LayoutResult;
}

void DashView::Relayout()
{
  nux::Geometry geo = GetGeometry();
  content_geo_ = GetBestFitGeometry(geo);

  if (Settings::Instance().GetFormFactor() == FormFactor::NETBOOK)
  {
    if (geo.width >= content_geo_.width && geo.height > content_geo_.height)
      content_geo_ = geo;
  }

  // kinda hacky, but it makes sure the content isn't so big that it throws
  // the bottom of the dash off the screen
  // not hugely happy with this, so FIXME
  lenses_layout_->SetMaximumHeight (content_geo_.height - search_bar_->GetGeometry().height - lens_bar_->GetGeometry().height);
  lenses_layout_->SetMinimumHeight (content_geo_.height - search_bar_->GetGeometry().height - lens_bar_->GetGeometry().height);

  layout_->SetMinMaxSize(content_geo_.width, content_geo_.height);

  dash::Style& style = dash::Style::Instance();

  // Minus the padding that gets added to the left
  float tile_width = style.GetTileWidth();
  style.SetDefaultNColumns(floorf((content_geo_.width - 32) / tile_width));

  ubus_manager_.SendMessage(UBUS_DASH_SIZE_CHANGED, g_variant_new("(ii)", content_geo_.width, content_geo_.height));

  QueueDraw();
}

// Gives us the width and height of the contents that will give us the best "fit",
// which means that the icons/views will not have unnecessary padding, everything will
// look tight
nux::Geometry DashView::GetBestFitGeometry(nux::Geometry const& for_geo)
{
  dash::Style& style = dash::Style::Instance();

  int width = 0, height = 0;
  int tile_width = style.GetTileWidth();
  int tile_height = style.GetTileHeight();
  int half = for_geo.width / 2;

  // if default dash size is bigger than half a screens worth of items, go for that.
  while ((width += tile_width) + (19 * 2) < half)
    ;

  width = MAX(width, tile_width * 6);

  width += 19 + 32 + dash::Style::FILTERS_LEFT_PADDING + dash::Style::FILTERS_RIGHT_PADDING; // add the left padding and the group plugin padding

  height = search_bar_->GetGeometry().height;
  height += tile_height * 3;
  height += (24 + 15) * 3; // adding three group headers
  height += lens_bar_->GetGeometry().height;
  height += 6; // account for padding in PlacesGroup

  if (for_geo.width > 800 && for_geo.height > 550)
  {
    width = MIN(width, for_geo.width-66);
    height = MIN(height, for_geo.height-24);
  }

  return nux::Geometry(0, 0, width, height);
}

void DashView::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  renderer_.DrawFull(gfx_context, content_geo_, GetAbsoluteGeometry(), GetGeometry());
}

void DashView::DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  renderer_.DrawInner(gfx_context, content_geo_, GetAbsoluteGeometry(), GetGeometry());
  
  if (IsFullRedraw())
  {
    nux::GetPainter().PushBackgroundStack();
    layout_->ProcessDraw(gfx_context, force_draw);
    nux::GetPainter().PopBackgroundStack();
  }
  else
  {
    layout_->ProcessDraw(gfx_context, force_draw);
  }
  
  renderer_.DrawInnerCleanup(gfx_context, content_geo_, GetAbsoluteGeometry(), GetGeometry());
}

void DashView::OnMouseButtonDown(int x, int y, unsigned long button, unsigned long key)
{
  if (!content_geo_.IsPointInside(x, y))
  {
    ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
  }
}

void DashView::OnActivateRequest(GVariant* args)
{
  glib::String uri;
  glib::String search_string;
  dash::HandledType handled_type;

  g_variant_get(args, "(sus)", &uri, &handled_type, &search_string);

  std::string id = AnalyseLensURI(uri.Str());

  lens_bar_->Activate(id);

  if ((id == "home.lens" && handled_type != GOTO_DASH_URI ) || !visible_)
    ubus_manager_.SendMessage(UBUS_DASH_EXTERNAL_ACTIVATION);
}

std::string DashView::AnalyseLensURI(std::string const& uri)
{
  impl::LensFilter filter = impl::parse_lens_uri(uri);

  if (!filter.filters.empty())
  {
    lens_views_[filter.id]->filters_expanded = true;
    // update the lens for each filter
    for (auto p : filter.filters) {
      UpdateLensFilter(filter.id, p.first, p.second);
    }
  }

  return filter.id;
}

void DashView::UpdateLensFilter(std::string lens_id, std::string filter_name, std::string value)
{
  if (lenses_.GetLens(lens_id))
  {
    Lens::Ptr lens = lenses_.GetLens(lens_id);

    Filters::Ptr filters = lens->filters;

    for (unsigned int i = 0; i < filters->count(); ++i)
    {
      Filter::Ptr filter = filters->FilterAtIndex(i);

      if (filter->id() == filter_name)
      {
        UpdateLensFilterValue(filter, value);
      }
    }
  }
}

void DashView::UpdateLensFilterValue(Filter::Ptr filter, std::string value)
{
  if (filter->renderer_name == "filter-radiooption")
  {
    RadioOptionFilter::Ptr radio = std::static_pointer_cast<RadioOptionFilter>(filter);
    for (auto option: radio->options())
    {
      if (option->id == value)
        option->active = true;
    }
  }
}

gboolean DashView::ResetSearchStateCb(gpointer data)
{
  DashView *self = static_cast<DashView*>(data);

  self->search_in_progress_ = false;
  self->activate_on_finish_ = false;
  self->searching_timeout_id_ = 0;

  return FALSE;
}

void DashView::OnSearchChanged(std::string const& search_string)
{
  LOG_DEBUG(logger) << "Search changed: " << search_string;
  if (active_lens_view_)
  {
    search_in_progress_ = true;
    // it isn't guaranteed that we get a SearchFinished signal, so we need
    // to make sure this isn't set even though we aren't doing any search
    if (searching_timeout_id_)
    {
      g_source_remove (searching_timeout_id_);
    }
    // 250ms for the Search method call, rest for the actual search
    searching_timeout_id_ = g_timeout_add (500, &DashView::ResetSearchStateCb, this);
  }
}

void DashView::OnLiveSearchReached(std::string const& search_string)
{
  LOG_DEBUG(logger) << "Live search reached: " << search_string;
  if (active_lens_view_)
  {
    active_lens_view_->search_string = search_string;
  }
}

void DashView::OnLensAdded(Lens::Ptr& lens)
{
  std::string id = lens->id;
  lens_bar_->AddLens(lens);

  LensView* view = new LensView(lens);
  AddChild(view);
  view->SetVisible(false);
  view->uri_activated.connect(sigc::mem_fun(this, &DashView::OnUriActivated));
  lenses_layout_->AddView(view, 1);
  lens_views_[lens->id] = view;

  lens->activated.connect(sigc::mem_fun(this, &DashView::OnUriActivatedReply));
  lens->search_finished.connect(sigc::mem_fun(this, &DashView::OnSearchFinished));
  lens->global_search_finished.connect(sigc::mem_fun(this, &DashView::OnGlobalSearchFinished));
}

void DashView::OnLensBarActivated(std::string const& id)
{
  if (lens_views_.find(id) == lens_views_.end())
  {
    LOG_WARN(logger) << "Unable to find Lens " << id;
    return;
  }

  LensView* view = active_lens_view_ = lens_views_[id];

  for (auto it: lens_views_)
  {
    bool id_matches = it.first == id;
    ViewType view_type = id_matches ? LENS_VIEW : (view == home_view_ ? HOME_VIEW : HIDDEN);
    it.second->SetVisible(id_matches);
    it.second->view_type = view_type;

    LOG_DEBUG(logger) << "Setting ViewType " << view_type
                      << " on '" << it.first << "'";
  }

  search_bar_->search_string = view->search_string;
  search_bar_->search_hint = view->lens()->search_hint;

  bool expanded = view->filters_expanded;
  search_bar_->showing_filters = expanded;

  nux::GetWindowCompositor().SetKeyFocusArea(default_focus());

  search_bar_->text_entry()->SelectAll();

  search_bar_->can_refine_search = view->can_refine_search();

  view->QueueDraw();
  QueueDraw();
}

void DashView::OnSearchFinished(Lens::Hints const& hints)
{
  Lens::Hints::const_iterator it;
  it = hints.find("no-results-hint");
  
  if (it != hints.end())
  {
    LOG_DEBUG(logger) << "We have no-results-hint: " << g_variant_get_string (it->second, NULL);
  }

  std::string search_string = search_bar_->search_string;
  if (active_lens_view_ && active_lens_view_->search_string == search_string)
  {
    search_bar_->SearchFinished();
    search_in_progress_ = false;
    if (activate_on_finish_)
      this->OnEntryActivated();
  }
}

void DashView::OnGlobalSearchFinished(Lens::Hints const& hints)
{
  if (active_lens_view_ == home_view_)
    OnSearchFinished(hints);
}

void DashView::OnUriActivated(std::string const& uri)
{
  last_activated_uri_ = uri;
}

void DashView::OnUriActivatedReply(std::string const& uri, HandledType type, Lens::Hints const&)
{
  // We don't want to close the dash if there was another activation pending
  if (type == NOT_HANDLED)
  {
    if (!DoFallbackActivation(uri))
      return;
  }
  else if (type == SHOW_DASH)
  {
    return;
  }

  ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
}

bool DashView::DoFallbackActivation(std::string const& fake_uri)
{
  size_t pos = fake_uri.find(":");
  std::string uri = fake_uri.substr(++pos);

  LOG_DEBUG(logger) << "Fallback activating " << uri;

  if (g_str_has_prefix(uri.c_str(), "application://"))
  {
    std::string appname = uri.substr(14);
    return LaunchApp(appname);
  }
  else if (g_str_has_prefix(uri.c_str(), "unity-runner://"))
  {
    std::string appname = uri.substr(15);
    return LaunchApp(appname);
  }
  else
    return gtk_show_uri(NULL, uri.c_str(), time(NULL), NULL);

  return false;
}

bool DashView::LaunchApp(std::string const& appname)
{
  GDesktopAppInfo* info;
  bool ret = false;
  char *id = g_strdup(appname.c_str());
  int i = 0;

  LOG_DEBUG(logger) << "Launching " << appname;

  while (id != NULL)
  {
    info = g_desktop_app_info_new(id);
    if (info != NULL)
    {
      GError* error = NULL;

      g_app_info_launch(G_APP_INFO(info), NULL, NULL, &error);
      if (error)
      {
        g_warning("Unable to launch %s: %s", id,  error->message);
        g_error_free(error);
      }
      else
        ret = true;
      g_object_unref(info);
      break;
    }

    /* Try to replace the next - with a / and do the lookup again.
     * If we set id=NULL we'll exit the outer loop */
    for (i = 0; ; i++)
    {
      if (id[i] == '-')
      {
        id[i] = '/';
        break;
      }
      else if (id[i] == '\0')
      {
        g_free(id);
        id = NULL;
        break;
      }
    }
  }

  g_free(id);
  return ret;
}

void DashView::DisableBlur()
{
  renderer_.DisableBlur();
}
void DashView::OnEntryActivated()
{
  if (!search_in_progress_)
  {
    active_lens_view_->ActivateFirst();
  }
  // delay the activation until we get the SearchFinished signal
  activate_on_finish_ = search_in_progress_;
}

// Keyboard navigation
bool DashView::AcceptKeyNavFocus()
{
  return false;
}

std::string const DashView::GetIdForShortcutActivation(std::string const& shortcut) const
{
  Lens::Ptr lens = lenses_.GetLensForShortcut(shortcut);
  if (lens)
    return lens->id;
  return "";
}

std::vector<char> DashView::GetAllShortcuts()
{
  std::vector<char> result;

  for (Lens::Ptr lens: lenses_.GetLenses())
  {
    std::string shortcut = lens->shortcut;
    if(shortcut.size() > 0)
      result.push_back(shortcut.at(0));
  }
  return result;
}

bool DashView::InspectKeyEvent(unsigned int eventType,
                               unsigned int key_sym,
                               const char* character)
{
  if ((eventType == nux::NUX_KEYDOWN) && (key_sym == NUX_VK_ESCAPE))
  {
    if (search_bar_->search_string == "")
      ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
    else
      search_bar_->search_string = "";

    return true;
  }
  return false;
}

nux::View* DashView::default_focus() const
{
  return search_bar_->text_entry();
}

// Introspectable
std::string DashView::GetName() const
{
  return "DashView";
}

void DashView::AddProperties(GVariantBuilder* builder)
{
  int num_rows = 1; // The search bar

  if (active_lens_view_)
    num_rows += active_lens_view_->GetNumRows();

  unity::variant::BuilderWrapper wrapper(builder);
  wrapper.add("num-rows", num_rows);
}

nux::Area * DashView::KeyNavIteration(nux::KeyNavDirection direction)
{
  // We don't want to eat the tab as it's used for IM stuff
  if (!search_bar_->im_active())
  {
    if (direction == KEY_NAV_TAB_NEXT)
      lens_bar_->ActivateNext();
    else if (direction == KEY_NAV_TAB_PREVIOUS)
      lens_bar_->ActivatePrevious();
  }
  return this;
}

void DashView::ProcessDndEnter()
{
  ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
}

Area* DashView::FindKeyFocusArea(unsigned int key_symbol,
      unsigned long x11_key_code,
      unsigned long special_keys_state)
{
  // Do what nux::View does, but if the event isn't a key navigation,
  // designate the text entry to process it.

  nux::KeyNavDirection direction = KEY_NAV_NONE;
  switch (x11_key_code)
  {
  case NUX_VK_UP:
    direction = KEY_NAV_UP;
    break;
  case NUX_VK_DOWN:
    direction = KEY_NAV_DOWN;
    break;
  case NUX_VK_LEFT:
    direction = KEY_NAV_LEFT;
    break;
  case NUX_VK_RIGHT:
    direction = KEY_NAV_RIGHT;
    break;
  case NUX_VK_LEFT_TAB:
    direction = KEY_NAV_TAB_PREVIOUS;
    break;
  case NUX_VK_TAB:
    direction = KEY_NAV_TAB_NEXT;
    break;
  case NUX_VK_ENTER:
  case NUX_KP_ENTER:
    // Not sure if Enter should be a navigation key
    direction = KEY_NAV_ENTER;
    break;
  default:
    direction = KEY_NAV_NONE;
    break;
  }

  if (has_key_focus_)
  {
    return this;
  }
  else if (direction == KEY_NAV_NONE || search_bar_->im_active)
  {
    // then send the event to the search entry
    return search_bar_->text_entry();
  }
  else if (next_object_to_key_focus_area_)
  {
    return next_object_to_key_focus_area_->FindKeyFocusArea(key_symbol, x11_key_code, special_keys_state);
  }
  return NULL;
}

}
}
