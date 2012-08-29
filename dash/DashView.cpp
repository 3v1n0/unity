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

#include "unity-shared/DashStyle.h"
#include "unity-shared/KeyboardUtil.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/PreviewStyle.h"

#include "Nux/NuxTimerTickSource.h"

namespace unity
{
namespace dash
{
namespace
{

nux::logging::Logger logger("unity.dash.view");
previews::Style preview_style;
}

// This is so we can access some protected members in nux::VLayout and
// break the natural key navigation path.
class DashLayout: public nux::VLayout
{
public:
  DashLayout(NUX_FILE_LINE_DECL)
    : nux::VLayout(NUX_FILE_LINE_PARAM)
    , area_(nullptr)
  {}

  void SetSpecialArea(nux::Area* area)
  {
    area_ = area;
  }

protected:
  nux::Area* KeyNavIteration(nux::KeyNavDirection direction)
  {
    if (direction == nux::KEY_NAV_DOWN && area_ &&  area_->HasKeyFocus())
      return nullptr;
    else
      return nux::VLayout::KeyNavIteration(direction);
  }

private:
  nux::Area* area_;
};

NUX_IMPLEMENT_OBJECT_TYPE(DashView);

DashView::DashView()
  : nux::View(NUX_TRACKER_LOCATION)
  , home_lens_(new HomeLens(_("Home"), _("Home screen"), _("Search")))
  , preview_container_(nullptr)
  , preview_displaying_(false)
  , preview_navigation_mode_(previews::Navigation::NONE)
  , active_lens_view_(0)
  , last_activated_uri_("")
  , search_in_progress_(false)
  , activate_on_finish_(false)
  , visible_(false)
{
  //SetRedirectRenderingToTexture(true); // render this view into an offscreen texture.
  tick_source_.reset(new nux::NuxTimerTickSource);
  animation_controller_.reset(new na::AnimationController(*tick_source_));

  renderer_.SetOwner(this);
  renderer_.need_redraw.connect([this] () {
    QueueDraw();
  });

  SetupViews();
  SetupUBusConnections();

  Settings::Instance().changed.connect(sigc::mem_fun(this, &DashView::Relayout));
  lenses_.lens_added.connect(sigc::mem_fun(this, &DashView::OnLensAdded));
  mouse_down.connect(sigc::mem_fun(this, &DashView::OnMouseButtonDown));
  preview_state_machine_.PreviewActivated.connect(sigc::mem_fun(this, &DashView::BuildPreview));
  Relayout();

  home_lens_->AddLenses(lenses_);
  home_lens_->search_finished.connect(sigc::mem_fun(this, &DashView::OnGlobalSearchFinished));
  lens_bar_->Activate("home.lens");
}

DashView::~DashView()
{
  // Do this explicitely, otherwise dee will complain about invalid access
  // to the lens models
  RemoveLayout();
}

void DashView::SetMonitorOffset(int x, int y)
{
  renderer_.x_offset = x;
  renderer_.y_offset = y;
}

void DashView::ClosePreview()
{
  if (preview_displaying_)
  {
    // nux::TexCoordXForm texxform;
    // nux::ObjectPtr<nux::IOpenGLBaseTexture> src_texture;

    // src_texture = layout_->BackupTexture();
    // nux::GetGraphicsDisplay()->GetGraphicsEngine()->QRP_GetCopyTexture(
    //   src_texture->GetWidth(), src_texture->GetHeight(),
    //   layout_copy_, src_texture,
    //   texxform, nux::color::White);

    animation_.Stop();
    // Set fade animation
    animation_.SetDuration(600);
    animation_.SetEasingCurve(na::EasingCurve(na::EasingCurve::Type::Linear));
    animation_.updated.connect(sigc::mem_fun(this, &DashView::FadeInCallBack));

    fade_in_value_ = 1.0f;
    animation_.SetStartValue(fade_in_value_);
    animation_.SetFinishValue(0.0f);
    animation_.Start();
  }

  preview_displaying_ = false;
  RemoveChild(preview_container_.GetPointer());
  preview_container_ = nullptr; // free resources
  preview_state_machine_.ClosePreview();
  QueueDraw();
}

void DashView::FadeOutCallBack(float const& fade_out_value)
{
  fade_out_value_ = fade_out_value;
  QueueDraw();
}

void DashView::FadeInCallBack(float const& fade_in_value)
{
  fade_in_value_ = fade_in_value;
  QueueDraw();
}

void DashView::BuildPreview(Preview::Ptr model)
{
  if (!preview_displaying_)
  {
    // Make a copy of this DashView backup texture.
    {
      nux::TexCoordXForm texxform;
      nux::ObjectPtr<nux::IOpenGLBaseTexture> src_texture;

      src_texture = layout_->BackupTexture();
      nux::GetGraphicsDisplay()->GetGraphicsEngine()->QRP_GetCopyTexture(
        src_texture->GetWidth(), src_texture->GetHeight(),
        layout_copy_, src_texture,
        texxform, nux::color::White);

      // Set fade animation
      animation_.SetDuration(600);
      animation_.SetEasingCurve(na::EasingCurve(na::EasingCurve::Type::Linear));
      animation_.updated.connect(sigc::mem_fun(this, &DashView::FadeOutCallBack));

      fade_out_value_ = 1.0f;
      animation_.SetStartValue(fade_out_value_);
      animation_.SetFinishValue(0.0f);
      animation_.Start();
    }

    preview_container_ = previews::PreviewContainer::Ptr(new previews::PreviewContainer());
    AddChild(preview_container_.GetPointer());
    preview_container_->Preview(model, previews::Navigation::NONE); // no swipe left or right
    //nux::GetWindowCompositor().SetKeyFocusArea(preview_container_.GetPointer());
    
    preview_container_->SetParentObject(this);
    preview_container_->SetGeometry(layout_->GetGeometry());
    preview_displaying_ = true;
 
    // connect to nav left/right signals to request nav left/right movement.
    preview_container_->navigate_left.connect([&] () {
      preview_state_machine_.Reset();
      preview_navigation_mode_ = previews::Navigation::LEFT;

      // sends a message to all result views, sending the the uri of the current preview result
      // and the unique id of the result view that should be handling the results
      ubus_manager_.SendMessage(UBUS_DASH_PREVIEW_NAVIGATION_REQUEST, g_variant_new("(iss)", -1, stored_preview_uri_identifier_.c_str(), stored_preview_unique_id_.c_str()));
    });

    preview_container_->navigate_right.connect([&] () {
      preview_state_machine_.Reset();
      preview_navigation_mode_ = previews::Navigation::RIGHT;
      
      // sends a message to all result views, sending the the uri of the current preview result
      // and the unique id of the result view that should be handling the results
      ubus_manager_.SendMessage(UBUS_DASH_PREVIEW_NAVIGATION_REQUEST, g_variant_new("(iss)", 1, stored_preview_uri_identifier_.c_str(), stored_preview_unique_id_.c_str()));
    });
  }
  else
  {
    // got a new preview whilst already displaying, we probably clicked a navigation button.
    preview_container_->Preview(model, preview_navigation_mode_); // TODO
  }

  if (G_LIKELY(preview_state_machine_.left_results() > 0 && preview_state_machine_.right_results() > 0))
    preview_container_->DisableNavButton(previews::Navigation::NONE);
  else if (preview_state_machine_.left_results() > 0)
    preview_container_->DisableNavButton(previews::Navigation::RIGHT);
  else if (preview_state_machine_.right_results() > 0)
    preview_container_->DisableNavButton(previews::Navigation::LEFT);
  else
    preview_container_->DisableNavButton(previews::Navigation::BOTH);

  QueueDraw();
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
  else if (active_lens_view_)
  {
    // careful here, the lens_view's view_type doesn't get reset when the dash
    // hides, but lens' view_type does, so we need to update the lens directly
    active_lens_view_->lens()->view_type = ViewType::LENS_VIEW;
  }

  // this will make sure the spinner animates if the search takes a while
  search_bar_->ForceSearchChanged();

  // if a preview is open, close it
  if (preview_displaying_) 
  {
    ClosePreview();
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
  dash::Style& style = dash::Style::Instance();

  layout_ = new nux::VLayout();
  layout_->SetLeftAndRightPadding(style.GetVSeparatorSize(), 0);
  layout_->SetTopAndBottomPadding(style.GetHSeparatorSize(), 0);
  SetLayout(layout_);
  layout_->SetRedirectRenderingToTexture(true);

  content_layout_ = new DashLayout(NUX_TRACKER_LOCATION);
  content_layout_->SetTopAndBottomPadding(style.GetDashViewTopPadding(), 0);
  layout_->AddLayout(content_layout_, 1, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);

  search_bar_layout_ = new nux::HLayout();
  search_bar_layout_->SetLeftAndRightPadding(style.GetSearchBarLeftPadding(), 0);
  content_layout_->AddLayout(search_bar_layout_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  search_bar_ = new SearchBar();
  AddChild(search_bar_);
  search_bar_->SetMinimumHeight(style.GetSearchBarHeight());
  search_bar_->SetMaximumHeight(style.GetSearchBarHeight());
  search_bar_->activated.connect(sigc::mem_fun(this, &DashView::OnEntryActivated));
  search_bar_->search_changed.connect(sigc::mem_fun(this, &DashView::OnSearchChanged));
  search_bar_->live_search_reached.connect(sigc::mem_fun(this, &DashView::OnLiveSearchReached));
  search_bar_->showing_filters.changed.connect([&] (bool showing) { if (active_lens_view_) active_lens_view_->filters_expanded = showing; QueueDraw(); });
  search_bar_layout_->AddView(search_bar_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  content_layout_->SetSpecialArea(search_bar_->show_filters());

  //search_bar_layout_->SetRedirectRenderingToTexture(true);


  lenses_layout_ = new nux::VLayout();
  content_layout_->AddView(lenses_layout_, 1, nux::MINOR_POSITION_LEFT);

  home_view_ = new LensView(home_lens_, nullptr);
  home_view_->uri_preview_activated.connect([&] (std::string const& uri, std::string const& unique_id) 
  {
    stored_preview_unique_id_ = unique_id;
    stored_preview_uri_identifier_ = uri;
  });

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

  ubus_manager_.RegisterInterest(UBUS_DASH_PREVIEW_INFO_PAYLOAD, [&] (GVariant *data) 
  {
    int position = -1;
    int results_to_the_left = 0;
    int results_to_the_right = 0;
    g_variant_get(data, "(iii)", &position, &results_to_the_left, &results_to_the_right);
    preview_state_machine_.SetSplitPosition(SplitPosition::CONTENT_AREA, position);
    preview_state_machine_.left_results = results_to_the_left;
    preview_state_machine_.right_results = results_to_the_right;
  });
}

long DashView::PostLayoutManagement (long LayoutResult)
{
  Relayout();
  return LayoutResult;
}

void DashView::Relayout()
{
  nux::Geometry const& geo = GetGeometry();
  content_geo_ = GetBestFitGeometry(geo);
  dash::Style& style = dash::Style::Instance();

  if (style.always_maximised)
  {
    if (geo.width >= content_geo_.width && geo.height > content_geo_.height)
      content_geo_ = geo;
  }


  // kinda hacky, but it makes sure the content isn't so big that it throws
  // the bottom of the dash off the screen
  // not hugely happy with this, so FIXME
  lenses_layout_->SetMaximumHeight (content_geo_.height - search_bar_->GetGeometry().height - lens_bar_->GetGeometry().height - style.GetDashViewTopPadding());
  lenses_layout_->SetMinimumHeight (content_geo_.height - search_bar_->GetGeometry().height - lens_bar_->GetGeometry().height - style.GetDashViewTopPadding());

  layout_->SetMinMaxSize(content_geo_.width, content_geo_.height);

  // Minus the padding that gets added to the left
  float tile_width = style.GetTileWidth();
  style.SetDefaultNColumns(floorf((content_geo_.width - 32) / tile_width));

  ubus_manager_.SendMessage(UBUS_DASH_SIZE_CHANGED, g_variant_new("(ii)", content_geo_.width, content_geo_.height));

  if (preview_displaying_)
    preview_container_->SetGeometry(layout_->GetGeometry());

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

  width += 20 + 40; // add the left padding and the group plugin padding

  height = search_bar_->GetGeometry().height;
  height += tile_height * 3;
  height += (style.GetPlacesGroupTopSpace() - 2 + 24 + 8) * 3; // adding three group headers
  height += 1*2; // hseparator height
  height += style.GetDashViewTopPadding();
  height += lens_bar_->GetGeometry().height;

  if (for_geo.width > 800 && for_geo.height > 550)
  {
    width = MIN(width, for_geo.width-66);
    height = MIN(height, for_geo.height-24);
  }

  return nux::Geometry(0, 0, width, height);
}

void DashView::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  renderer_.DrawFull(graphics_engine, content_geo_, GetAbsoluteGeometry(), GetGeometry());
  
  // we only do this because the previews don't redraw correctly right now, so we have to force
  // a full redraw every frame. performance sucks but we'll fix it post FF

  if (preview_displaying_)
  {
    if (layout_copy_.IsValid())
    {
      graphics_engine.PushClippingRectangle(layout_->GetGeometry());
      graphics_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      nux::TexCoordXForm texxform;

      texxform.uoffset = (search_bar_layout_->GetX() -layout_->GetX())/(float)layout_->GetWidth();
      texxform.voffset = (search_bar_layout_->GetY() -layout_->GetY())/(float)layout_->GetHeight();

      texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

      graphics_engine.QRP_1Tex(
        search_bar_layout_->GetX(),
        search_bar_layout_->GetY() - (1-fade_out_value_)*(search_bar_layout_->GetHeight() + 10),
        search_bar_layout_->GetWidth(),
        search_bar_layout_->GetHeight(),
        layout_copy_, texxform,
        nux::Color(fade_out_value_, fade_out_value_, fade_out_value_, fade_out_value_)
        );

      int filter_width = 10;
      if (active_lens_view_ && active_lens_view_->filters_expanded)
      {
        texxform.uoffset = (active_lens_view_->filter_bar()->GetX() -layout_->GetX())/(float)layout_->GetWidth();
        texxform.voffset = (active_lens_view_->filter_bar()->GetY() -layout_->GetY())/(float)layout_->GetHeight();

        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

        graphics_engine.QRP_1Tex(
          active_lens_view_->filter_bar()->GetX() + (1-fade_out_value_)*(active_lens_view_->filter_bar()->GetWidth() + 10),
          active_lens_view_->filter_bar()->GetY(),
          active_lens_view_->filter_bar()->GetWidth(),
          active_lens_view_->filter_bar()->GetHeight(),
          layout_copy_, texxform,
          nux::Color(fade_out_value_, fade_out_value_, fade_out_value_, fade_out_value_)
          );
        filter_width += active_lens_view_->filter_bar()->GetWidth();
      }  

      // Center part 
      texxform.uoffset = (home_view_->GetX() -layout_->GetX())/(float)layout_->GetWidth();
      texxform.voffset = (home_view_->GetY() -layout_->GetY())/(float)layout_->GetHeight();

      texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

      graphics_engine.QRP_1Tex(
        home_view_->GetX(),
        home_view_->GetY(),
        home_view_->GetWidth() - filter_width,
        home_view_->GetHeight(),
        layout_copy_, texxform,
        nux::Color(fade_out_value_, fade_out_value_, fade_out_value_, fade_out_value_)
        );

      graphics_engine.GetRenderStates().SetBlend(false);
      graphics_engine.PopClippingRectangle();
    }

    preview_container_->ProcessDraw(graphics_engine, true); 
  }
  else if (fade_in_value_ != 0.0f)
  {
    if (layout_copy_.IsValid())
    {
      graphics_engine.PushClippingRectangle(layout_->GetGeometry());
      graphics_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      nux::TexCoordXForm texxform;

      texxform.uoffset = (search_bar_layout_->GetX() -layout_->GetX())/(float)layout_->GetWidth();
      texxform.voffset = (search_bar_layout_->GetY() -layout_->GetY())/(float)layout_->GetHeight();

      texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

      graphics_engine.QRP_1Tex(
        search_bar_layout_->GetX(),
        search_bar_layout_->GetY() - (fade_in_value_)*(search_bar_layout_->GetHeight() + 10),
        search_bar_layout_->GetWidth(),
        search_bar_layout_->GetHeight(),
        layout_copy_, texxform,
        nux::Color(1.0f - fade_in_value_, 1.0f - fade_in_value_, 1.0f - fade_in_value_, 1.0f - fade_in_value_)
        );

      int filter_width = 10;
      if (active_lens_view_ && active_lens_view_->filters_expanded)
      {
        texxform.uoffset = (active_lens_view_->filter_bar()->GetX() -layout_->GetX())/(float)layout_->GetWidth();
        texxform.voffset = (active_lens_view_->filter_bar()->GetY() -layout_->GetY())/(float)layout_->GetHeight();

        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

        graphics_engine.QRP_1Tex(
          active_lens_view_->filter_bar()->GetX() + (fade_in_value_)*(active_lens_view_->filter_bar()->GetWidth() + 10),
          active_lens_view_->filter_bar()->GetY(),
          active_lens_view_->filter_bar()->GetWidth(),
          active_lens_view_->filter_bar()->GetHeight(),
          layout_copy_, texxform,
          nux::Color(1.0f - fade_in_value_, 1.0f - fade_in_value_, 1.0f - fade_in_value_, 1.0f - fade_in_value_)
          );
        filter_width += active_lens_view_->filter_bar()->GetWidth();
      }  

      // Center part 
      texxform.uoffset = (home_view_->GetX() -layout_->GetX())/(float)layout_->GetWidth();
      texxform.voffset = (home_view_->GetY() -layout_->GetY())/(float)layout_->GetHeight();

      texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);


      graphics_engine.QRP_1Tex(
        home_view_->GetX(),
        home_view_->GetY(),
        home_view_->GetWidth() - filter_width,
        home_view_->GetHeight(),
        layout_copy_, texxform,
        nux::Color(1.0f - fade_in_value_, 1.0f - fade_in_value_, 1.0f - fade_in_value_, 1.0f - fade_in_value_)
        );

      graphics_engine.GetRenderStates().SetBlend(false);
      graphics_engine.PopClippingRectangle();
    }    
  }
}

void DashView::DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  renderer_.DrawInner(gfx_context, content_geo_, GetAbsoluteGeometry(), GetGeometry());

  if (!preview_displaying_ && (fade_in_value_ == 0.0f))
  {
    // nux::Geometry geo = GetGeometry();
    // geo.width -= 340;
    // geo.height -= 20;

    gfx_context.PushClippingRectangle(layout_->GetGeometry());
    nux::GetPainter().PaintBackground(gfx_context, layout_->GetGeometry());
    gfx_context.PopClippingRectangle();
  }

  if (IsFullRedraw())
    nux::GetPainter().PushBackgroundStack();
  
  if (preview_displaying_)
   preview_container_->ProcessDraw(gfx_context, (!force_draw) ? IsFullRedraw() : force_draw);
  else if (fade_in_value_ == 0.0f)
    layout_->ProcessDraw(gfx_context, force_draw);
    
  if (IsFullRedraw())
    nux::GetPainter().PopBackgroundStack();

  renderer_.DrawInnerCleanup(gfx_context, content_geo_, GetAbsoluteGeometry(), GetGeometry());
}

  void DashView::OnMouseButtonDown(int x, int y, unsigned long button, unsigned long key)
  {
    dash::Style& style = dash::Style::Instance();
    nux::Geometry geo(content_geo_);

  if (Settings::Instance().GetFormFactor() == FormFactor::DESKTOP)
  {
    geo.width += style.GetDashRightTileWidth();
    geo.height += style.GetDashBottomTileHeight();
  }

  if (!geo.IsPointInside(x, y))
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

  std::string id(AnalyseLensURI(uri.Str()));

  // we got an activation request, we should probably close the preview
  if (preview_displaying_)
  {
    ClosePreview();
  }

  if (!visible_)
  {
    lens_bar_->Activate(id);
    ubus_manager_.SendMessage(UBUS_DASH_EXTERNAL_ACTIVATION);
  }
  else if (/* visible_ && */ handled_type == NOT_HANDLED)
  {
    ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
  }
  else if (/* visible_ && */ handled_type == GOTO_DASH_URI)
  {
    lens_bar_->Activate(id);
  }
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

void DashView::OnSearchChanged(std::string const& search_string)
{
  LOG_DEBUG(logger) << "Search changed: " << search_string;
  if (active_lens_view_)
  {
    search_in_progress_ = true;
    // it isn't guaranteed that we get a SearchFinished signal, so we need
    // to make sure this isn't set even though we aren't doing any search
    // 250ms for the Search method call, rest for the actual search
    searching_timeout_.reset(new glib::Timeout(500, [&] () {
      search_in_progress_ = false;
      activate_on_finish_ = false;
      return false;
    }));

    // 150ms to hide the no reults message if its take a while to return results
    hide_message_delay_.reset(new glib::Timeout(150, [&] () {
      active_lens_view_->HideResultsMessage();
      return false;
    }));
  }
}

void DashView::OnLiveSearchReached(std::string const& search_string)
{
  LOG_DEBUG(logger) << "Live search reached: " << search_string;
  if (active_lens_view_)
  {
    active_lens_view_->PerformSearch(search_string);
  }
}

void DashView::OnLensAdded(Lens::Ptr& lens)
{
  std::string id = lens->id;
  lens_bar_->AddLens(lens);

  LensView* view = new LensView(lens, search_bar_->show_filters());
  AddChild(view);
  view->SetVisible(false);
  view->uri_activated.connect(sigc::mem_fun(this, &DashView::OnUriActivated));
  view->uri_preview_activated.connect([&] (std::string const& uri, std::string const& unique_id) 
  {
    LOG_DEBUG(logger) << "got unique id from preview activation: " << unique_id;
    stored_preview_unique_id_ = unique_id;
    stored_preview_uri_identifier_ = uri;
  });

  lenses_layout_->AddView(view, 1);
  lens_views_[lens->id] = view;

  lens->activated.connect(sigc::mem_fun(this, &DashView::OnUriActivatedReply));
  lens->search_finished.connect(sigc::mem_fun(this, &DashView::OnSearchFinished));
  lens->connected.changed.connect([&] (bool value)
  {
    std::string const& search_string = search_bar_->search_string;
    if (value && lens->search_in_global && active_lens_view_ == home_view_
        && !search_string.empty())
    {
      // force a (global!) search with the correct string
      lens->GlobalSearch(search_bar_->search_string);
    }
  });

  // Hook up to the new preview infrastructure
  lens->preview_ready.connect([&] (std::string const& uri, Preview::Ptr model)
  {
    LOG_DEBUG(logger) << "Got preview for: " << uri;
    preview_state_machine_.ActivatePreview(model); // this does not immediately display a preview - we now wait.
  });

  // global search done is handled by the home lens, no need to connect to it
  // BUT, we will special case global search finished coming from
  // the applications lens, because we want to be able to launch applications
  // immediately without waiting for the search finished signal which will
  // be delayed by all the lenses we're searching
  if (id == "applications.lens")
  {
    lens->global_search_finished.connect(sigc::mem_fun(this, &DashView::OnAppsGlobalSearchFinished));
  }
}

void DashView::OnLensBarActivated(std::string const& id)
{
  if (lens_views_.find(id) == lens_views_.end())
  {
    LOG_WARN(logger) << "Unable to find Lens " << id;
    return;
  }

  LensView* view = active_lens_view_ = lens_views_[id];
  view->JumpToTop();

  for (auto it: lens_views_)
  {
    bool id_matches = it.first == id;
    ViewType view_type = id_matches ? LENS_VIEW : (view == home_view_ ? HOME_VIEW : HIDDEN);
    it.second->SetVisible(id_matches);
    it.second->view_type = view_type;

    LOG_DEBUG(logger) << "Setting ViewType " << view_type
                      << " on '" << it.first << "'";
  }

  search_bar_->SetVisible(true);
  QueueRelayout();
  search_bar_->search_string = view->search_string;
  search_bar_->search_hint = view->lens()->search_hint;
  // lenses typically return immediately from Search() if the search query
  // doesn't change, so SearchFinished will be called in a few ms
  // FIXME: if we're forcing a search here, why don't we get rid of view types?
  search_bar_->ForceSearchChanged();

  bool expanded = view->filters_expanded;
  search_bar_->showing_filters = expanded;

  nux::GetWindowCompositor().SetKeyFocusArea(default_focus());

  search_bar_->text_entry()->SelectAll();
  search_bar_->can_refine_search = view->can_refine_search();
  hide_message_delay_.reset();

  view->QueueDraw();
  QueueDraw();
}

void DashView::OnSearchFinished(Lens::Hints const& hints)
{
  hide_message_delay_.reset();

  if (active_lens_view_ == NULL) return;

  active_lens_view_->CheckNoResults(hints);
  std::string const& search_string = search_bar_->search_string;

  if (active_lens_view_->search_string == search_string)
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

void DashView::OnAppsGlobalSearchFinished(Lens::Hints const& hints)
{
  if (active_lens_view_ == home_view_)
  {
    /* HACKITY HACK! We're resetting the state of search_in_progress when
     * doing searches in the home lens and we get results from apps lens.
     * This way typing a search query and pressing enter immediately will
     * wait for the apps lens results and will run correct application.
     * See lp:966417 and lp:856206 for more info about why we do this.
     */
    search_in_progress_ = false;
    if (activate_on_finish_)
      this->OnEntryActivated();
  }
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
    if (preview_displaying_)
      ClosePreview();
    else if (search_bar_->search_string != "")
      search_bar_->search_string = "";
    else
      ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
    
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
  dash::Style& style = dash::Style::Instance();
  int num_rows = 1; // The search bar

  if (active_lens_view_)
    num_rows += active_lens_view_->GetNumRows();

  std::string form_factor("unknown");

  if (Settings::Instance().GetFormFactor() == FormFactor::NETBOOK)
    form_factor = "netbook";
  else if (Settings::Instance().GetFormFactor() == FormFactor::DESKTOP)
    form_factor = "desktop";
  else if (Settings::Instance().GetFormFactor() == FormFactor::TV)
    form_factor = "tv";

  unity::variant::BuilderWrapper wrapper(builder);
  wrapper.add(nux::Geometry(GetAbsoluteX(), GetAbsoluteY(), content_geo_.width, content_geo_.height));
  wrapper.add("num_rows", num_rows);
  wrapper.add("form_factor", form_factor);
  wrapper.add("right-border-width", style.GetDashRightTileWidth());
  wrapper.add("bottom-border-height", style.GetDashBottomTileHeight());
  wrapper.add("preview_displaying", preview_displaying_);
}

nux::Area* DashView::KeyNavIteration(nux::KeyNavDirection direction)
{
  if (preview_displaying_)
  {
    preview_container_->KeyNavIteration(direction);
  }
  else if (direction == nux::KEY_NAV_DOWN && search_bar_ && active_lens_view_)
  {
    auto show_filters = search_bar_->show_filters();
    auto fscroll_view = active_lens_view_->fscroll_view();

    if (show_filters && show_filters->HasKeyFocus())
    {
      if (fscroll_view->IsVisible() && fscroll_view)
        return fscroll_view->KeyNavIteration(direction);
      else
        return active_lens_view_->KeyNavIteration(direction);
    }
  }
  return this;
}

void DashView::ProcessDndEnter()
{
  ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
}

nux::Area* DashView::FindKeyFocusArea(unsigned int key_symbol,
                                      unsigned long x11_key_code,
                                      unsigned long special_keys_state)
{
  // Only care about states of Alt, Ctrl, Super, Shift, not the lock keys
  special_keys_state &= (nux::NUX_STATE_ALT | nux::NUX_STATE_CTRL |
                         nux::NUX_STATE_SUPER | nux::NUX_STATE_SHIFT);

  // Do what nux::View does, but if the event isn't a key navigation,
  // designate the text entry to process it.

  using namespace nux;
  nux::KeyNavDirection direction = KEY_NAV_NONE;
  bool ctrl = (special_keys_state & NUX_STATE_CTRL);

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
  case NUX_VK_PAGE_UP:
    direction = KEY_NAV_TAB_PREVIOUS;
    break;
  case NUX_VK_TAB:
  case NUX_VK_PAGE_DOWN:
    direction = KEY_NAV_TAB_NEXT;
    break;
  case NUX_VK_ENTER:
  case NUX_KP_ENTER:
    // Not sure if Enter should be a navigation key
    direction = KEY_NAV_ENTER;
    break;
  case NUX_VK_F4:
    // Maybe we should not do it here, but it needs to be checked where
    // we are able to know if alt is pressed.
    if (special_keys_state == NUX_STATE_ALT)
    {
      ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
    }
    break;
  default:
    direction = KEY_NAV_NONE;
  }

  // We should not do it here, but I really don't want to make DashView
  // focusable and I'm not able to know if ctrl is pressed in
  // DashView::KeyNavIteration.
   nux::InputArea* focus_area = nux::GetWindowCompositor().GetKeyFocusArea();

  if (direction != KEY_NAV_NONE && key_symbol == nux::NUX_KEYDOWN && !search_bar_->im_preedit)
  {
    std::list<nux::Area*> tabs;
    for (auto category : active_lens_view_->categories())
    {
      if (category->IsVisible())
        tabs.push_back(category);
    }

    if (search_bar_ && search_bar_->show_filters() &&
        search_bar_->show_filters()->IsVisible())
    {
      tabs.push_back(search_bar_->show_filters());
    }

    if (active_lens_view_->filter_bar() && active_lens_view_->fscroll_view() &&
        active_lens_view_->fscroll_view()->IsVisible())
    {
      for (auto filter : active_lens_view_->filter_bar()->GetLayout()->GetChildren())
      {
        tabs.push_back(filter);
      }
    }

    if (direction == KEY_NAV_TAB_PREVIOUS)
    {
      if (ctrl)
      {
        lens_bar_->ActivatePrevious();
      }
      else
      {
        auto rbegin = tabs.rbegin();
        auto rend = tabs.rend();

        bool use_the_prev = false;
        for (auto tab = rbegin; tab != rend; ++tab)
        {
          const auto& tab_ptr = *tab;

          if (use_the_prev)
            return tab_ptr;

          if (focus_area)
            use_the_prev = focus_area->IsChildOf(tab_ptr);
        }

        for (auto tab = rbegin; tab != rend; ++tab)
          return *tab;
      }
    }
    else if (direction == KEY_NAV_TAB_NEXT)
    {
      if (ctrl)
      {
        lens_bar_->ActivateNext();
      }
      else
      {
        bool use_the_next = false;
        for (auto tab : tabs)
        {
          if (use_the_next)
            return tab;

          if (focus_area)
            use_the_next = focus_area->IsChildOf(tab);
        }

        for (auto tab : tabs)
          return tab;
      }
    }
  }

  bool search_key = false;

  if (direction == KEY_NAV_NONE)
  {
    if (ui::KeyboardUtil::IsPrintableKeySymbol(x11_key_code) ||
        ui::KeyboardUtil::IsMoveKeySymbol(x11_key_code))
    {
      search_key = true;
    }
  }

  if (search_key || search_bar_->im_preedit)
  {
    // then send the event to the search entry
    return search_bar_->text_entry();
  }
  else if (next_object_to_key_focus_area_)
  {
    return next_object_to_key_focus_area_->FindKeyFocusArea(key_symbol, x11_key_code, special_keys_state);
  }

  return nullptr;
}

nux::Area* DashView::FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
{
  nux::Area* view = nullptr;
  if (preview_displaying_)
  {
    nux::Point newpos = mouse_position;
    view = dynamic_cast<nux::Area*>(preview_container_.GetPointer())->FindAreaUnderMouse(newpos, event_type);
  }
  else
  {
    view = View::FindAreaUnderMouse(mouse_position, event_type);
  }

  return (view == nullptr) ? this : view;
}

}
}
