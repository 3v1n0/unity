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
 *              Nick Dedekind <nick.dedekind@canonical.com>
 */


#include "DashView.h"
#include "DashViewPrivate.h"
#include "FilterExpanderLabel.h"
#include "MultiMonitor.h"

#include <math.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <NuxCore/Logger.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/RadioOptionFilter.h>
#include <UnityCore/PaymentPreview.h>

#include "unity-shared/DashStyle.h"
#include "unity-shared/KeyboardUtil.h"
#include "unity-shared/PreviewStyle.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/WindowManager.h"

namespace unity
{
namespace dash
{
DECLARE_LOGGER(logger, "unity.dash.view");
namespace
{
previews::Style preview_style;

const int PREVIEW_ANIMATION_LENGTH = 250;

const int DASH_TILE_HORIZONTAL_COUNT = 6;
const int DASH_DEFAULT_CATEGORY_COUNT = 3;
const int DASH_RESULT_RIGHT_PAD = 35;
const int GROUP_HEADING_HEIGHT = 24;

const int PREVIEW_ICON_SPLIT_OFFSCREEN_OFFSET = 10;

const int MAX_ENTRY_ACTIVATE_WAIT_TIMEOUT = 1000;
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

class DashContentView : public nux::View
{
public:
  DashContentView(NUX_FILE_LINE_DECL):View(NUX_FILE_LINE_PARAM)
  {
    SetRedirectRenderingToTexture(true);
  }

  void Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
  {}

  void DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
  {
    if (GetLayout())
      GetLayout()->ProcessDraw(graphics_engine, force_draw);
  }

};

NUX_IMPLEMENT_OBJECT_TYPE(DashView);

DashView::DashView(Scopes::Ptr const& scopes, ApplicationStarter::Ptr const& application_starter)
  : nux::View(NUX_TRACKER_LOCATION)
  , scopes_(scopes)
  , application_starter_(application_starter)
  , preview_container_(nullptr)
  , preview_displaying_(false)
  , preview_navigation_mode_(previews::Navigation::NONE)
  , last_activated_timestamp_(0)
  , search_in_progress_(false)
  , activate_on_finish_(false)
  , visible_(false)
  , opening_column_x_(-1)
  , opening_row_y_(-1)
  , opening_column_width_(0)
  , opening_row_height_(0)
  , animate_split_value_(0.0)
  , animate_preview_container_value_(0.0)
  , animate_preview_value_(0.0)
  , overlay_window_buttons_(new OverlayWindowButtons())
{
  renderer_.SetOwner(this);
  renderer_.need_redraw.connect([this] () {
    QueueDraw();
  });

  SetupViews();
  SetupUBusConnections();

  AddChild(overlay_window_buttons_.GetPointer());

  mouse_down.connect(sigc::mem_fun(this, &DashView::OnMouseButtonDown));
  preview_state_machine_.PreviewActivated.connect(sigc::mem_fun(this, &DashView::BuildPreview));
  Relayout();

  if (scopes_)
  {
    scopes_->scope_added.connect(sigc::mem_fun(this, &DashView::OnScopeAdded));
    scopes_->LoadScopes();
  }

  // If nux resets the focus, we need to set it back to the default focus item.
  key_nav_focus_change_connection_ = nux::GetWindowCompositor().key_nav_focus_change.connect([this](nux::Area *area, bool has_focus, nux::KeyNavDirection direction) {
    if (visible_ && !area)
      nux::GetWindowCompositor().SetKeyFocusArea(default_focus());
  });
}

DashView::~DashView()
{
  // Do this explicitely, otherwise dee will complain about invalid access
  // to the scope models
  RemoveLayout();
}

void DashView::SetMonitorOffset(int x, int y)
{
  renderer_.x_offset = x;
  renderer_.y_offset = y;
}

bool DashView::IsCommandLensOpen() const
{
  return (scope_bar_->GetActiveScopeId() == "commands.scope");
}


void DashView::OnResultActivated(ResultView::ActivateType type, LocalResult const& local_result, GVariant* data, std::string const& unique_id)
{
  last_activated_result_ = local_result;
  stored_activated_unique_id_ = unique_id;

  if (data)
  {
    // Update positioning information.
    int column_x = -1;
    int row_y = -1;
    int column_width = 0;
    int row_height = 0;
    int results_to_the_left = 0;
    int results_to_the_right = 0;
    g_variant_get(data, "(tiiiiii)", &last_activated_timestamp_, &column_x, &row_y, &column_width, &row_height, &results_to_the_left, &results_to_the_right);

    preview_state_machine_.SetSplitPosition(SplitPosition::CONTENT_AREA, row_y);
    preview_state_machine_.left_results = results_to_the_left;
    preview_state_machine_.right_results = results_to_the_right;

    opening_column_x_ = column_x;
    opening_row_y_ = row_y;
    opening_column_width_ = column_width;
    opening_row_height_ = row_height;
  }

  // we want immediate preview reaction on first opening.
  if (type == ResultView::ActivateType::PREVIEW && !preview_displaying_)
  {
    BuildPreview(Preview::Ptr(nullptr));
  }
}

void DashView::BuildPreview(Preview::Ptr model)
{
  if (!preview_displaying_)
  {
    StartPreviewAnimation();

    content_view_->SetPresentRedirectedView(false);
    preview_scope_view_ = active_scope_view_;
    if (preview_scope_view_)
    {
      preview_scope_view_->ForceCategoryExpansion(stored_activated_unique_id_, true);
      preview_scope_view_->EnableResultTextures(true);
      preview_scope_view_->PushFilterExpansion(false);
    }

    if (!preview_container_)
    {
      preview_container_ = previews::PreviewContainer::Ptr(new previews::PreviewContainer());
      preview_container_->SetRedirectRenderingToTexture(true);
      AddChild(preview_container_.GetPointer());
      preview_container_->SetParentObject(this);
    }
    preview_container_->Preview(model, previews::Navigation::NONE); // no swipe left or right

    preview_container_->SetGeometry(scopes_layout_->GetGeometry());
    preview_displaying_ = true;

    // connect to nav left/right signals to request nav left/right movement.
    preview_container_->navigate_left.connect([this] () {
      preview_navigation_mode_ = previews::Navigation::LEFT;

      // sends a message to all result views, sending the the uri of the current preview result
      // and the unique id of the result view that should be handling the results
      ubus_manager_.SendMessage(UBUS_DASH_PREVIEW_NAVIGATION_REQUEST, g_variant_new("(ivs)", -1, g_variant_ref(last_activated_result_.Variant()), stored_activated_unique_id_.c_str()));
    });

    preview_container_->navigate_right.connect([this] () {
      preview_navigation_mode_ = previews::Navigation::RIGHT;

      // sends a message to all result views, sending the the uri of the current preview result
      // and the unique id of the result view that should be handling the results
      ubus_manager_.SendMessage(UBUS_DASH_PREVIEW_NAVIGATION_REQUEST, g_variant_new("(ivs)", 1, g_variant_ref(last_activated_result_.Variant()), stored_activated_unique_id_.c_str()));
    });

    preview_container_->request_close.connect([this] () { ClosePreview(); });
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

void DashView::ClosePreview()
{
  if (preview_displaying_)
  {
    EndPreviewAnimation();

    preview_displaying_ = false;
  }

  preview_navigation_mode_ = previews::Navigation::NONE;

  // re-focus dash view component.
  nux::GetWindowCompositor().SetKeyFocusArea(default_focus());
  QueueDraw();
}

void DashView::StartPreviewAnimation()
{
  // We use linear animations so we can easily control when the next animation in the sequence starts.
  // The animation curve is caluclated separately.

  preview_animation_.reset();
  preview_container_animation_.reset();

  // Dash Split Open Animation
  split_animation_.reset(new na::AnimateValue<float>());
  split_animation_->SetDuration((1.0f - animate_split_value_) * PREVIEW_ANIMATION_LENGTH);
  split_animation_->SetStartValue(animate_split_value_);
  split_animation_->SetFinishValue(1.0f);
  split_animation_->SetEasingCurve(na::EasingCurve(na::EasingCurve::Type::Linear));
  split_animation_->updated.connect([this](float const& linear_split_animate_value)
  {
    static na::EasingCurve split_animation_curve(na::EasingCurve::Type::InQuad);

    animate_split_value_ = split_animation_curve.ValueForProgress(linear_split_animate_value);
    QueueDraw();

    // time to start the preview container animation?
    if (linear_split_animate_value >= 0.5f && !preview_container_animation_)
    {
      // Preview Container Close Animation
      preview_container_animation_.reset(new na::AnimateValue<float>());
      preview_container_animation_->SetDuration((1.0f - animate_preview_container_value_) * PREVIEW_ANIMATION_LENGTH);
      preview_container_animation_->SetStartValue(animate_preview_container_value_);
      preview_container_animation_->SetFinishValue(1.0f);
      preview_container_animation_->SetEasingCurve(na::EasingCurve(na::EasingCurve::Type::Linear));
      preview_container_animation_->updated.connect([this](float const& linear_preview_container_animate_value)
      {
        static na::EasingCurve preview_container_animation_curve(na::EasingCurve::Type::InQuad);

        animate_preview_container_value_ = preview_container_animation_curve.ValueForProgress(linear_preview_container_animate_value);
        QueueDraw();

        // time to start the preview animation?
        if (linear_preview_container_animate_value >= 0.9f && !preview_animation_)
        {
          // Preview Close Animation
          preview_animation_.reset(new na::AnimateValue<float>());
          preview_animation_->SetDuration((1.0f - animate_preview_value_) * PREVIEW_ANIMATION_LENGTH);
          preview_animation_->SetStartValue(animate_preview_value_);
          preview_animation_->SetFinishValue(1.0f);
          preview_animation_->SetEasingCurve(na::EasingCurve(na::EasingCurve::Type::Linear));
          preview_animation_->updated.connect([this](float const& linear_preview_animate_value)
          {
            animate_preview_value_ = linear_preview_animate_value;
            QueueDraw();
          });

          preview_animation_->finished.connect(sigc::mem_fun(this, &DashView::OnPreviewAnimationFinished));
          preview_animation_->Start();
        }
      });

      preview_container_animation_->finished.connect(sigc::mem_fun(this, &DashView::OnPreviewAnimationFinished));
      preview_container_animation_->Start();
    }

    if (preview_scope_view_)
      preview_scope_view_->SetResultsPreviewAnimationValue(animate_split_value_);
  });

  split_animation_->finished.connect(sigc::mem_fun(this, &DashView::OnPreviewAnimationFinished));
  split_animation_->Start();
}

void DashView::EndPreviewAnimation()
{
  // We use linear animations so we can easily control when the next animation in the sequence starts.
  // The animation curve is caluclated separately.

  split_animation_.reset();
  preview_container_animation_.reset();

  // Preview Close Animation
  preview_animation_.reset(new na::AnimateValue<float>());
  preview_animation_->SetDuration(animate_preview_value_ * PREVIEW_ANIMATION_LENGTH);
  preview_animation_->SetStartValue(1.0f - animate_preview_value_);
  preview_animation_->SetFinishValue(1.0f);
  preview_animation_->SetEasingCurve(na::EasingCurve(na::EasingCurve::Type::Linear));
  preview_animation_->updated.connect([this](float const& preview_value)
  {
    animate_preview_value_ = 1.0f - preview_value;
    QueueDraw();

    // time to stop the split?
    if (preview_value >= 0.9f && !preview_container_animation_)
    {
      // Preview Container Close Animation
      preview_container_animation_.reset(new na::AnimateValue<float>());
      preview_container_animation_->SetDuration(animate_preview_container_value_ * PREVIEW_ANIMATION_LENGTH);
      preview_container_animation_->SetStartValue(1.0f - animate_preview_container_value_);
      preview_container_animation_->SetFinishValue(1.0f);
      preview_container_animation_->SetEasingCurve(na::EasingCurve(na::EasingCurve::Type::Linear));
      preview_container_animation_->updated.connect([this](float const& linear_preview_container_animate_value)
      {
        static na::EasingCurve preview_container_animation_curve(na::EasingCurve::Type::InQuad);

        animate_preview_container_value_ = 1.0f - preview_container_animation_curve.ValueForProgress(linear_preview_container_animate_value);
        QueueDraw();

        if (linear_preview_container_animate_value >= 0.5f && !split_animation_)
        {
          // Dash Split Close Animation
          split_animation_.reset(new na::AnimateValue<float>());
          split_animation_->SetDuration(animate_split_value_ * PREVIEW_ANIMATION_LENGTH);
          split_animation_->SetStartValue(1.0f - animate_split_value_);
          split_animation_->SetFinishValue(1.0f);
          split_animation_->SetEasingCurve(na::EasingCurve(na::EasingCurve::Type::Linear));
          split_animation_->updated.connect([this](float const& linear_split_animate_value)
          {
            static na::EasingCurve split_animation_curve(na::EasingCurve::Type::InQuad);

            animate_split_value_ = 1.0f - split_animation_curve.ValueForProgress(linear_split_animate_value);
            QueueDraw();
          });
          split_animation_->finished.connect(sigc::mem_fun(this, &DashView::OnPreviewAnimationFinished));
          split_animation_->Start();

          // if (preview_scope_view_)
          //   preview_scope_view_->PopFilterExpansion();
        }
      });

      preview_container_animation_->finished.connect(sigc::mem_fun(this, &DashView::OnPreviewAnimationFinished));
      preview_container_animation_->Start();
    }
  });

  preview_animation_->finished.connect(sigc::mem_fun(this, &DashView::OnPreviewAnimationFinished));
  preview_animation_->Start();
}

void DashView::OnPreviewAnimationFinished()
{
  if (animate_preview_value_ != 0.0f || animate_preview_container_value_ != 0.0f || animate_split_value_ != 0.0f)
    return;

  // preview close finished.
  if (preview_container_)
  {
    RemoveChild(preview_container_.GetPointer());
    preview_container_->UnParentObject();
    preview_container_.Release(); // free resources
    preview_state_machine_.ClosePreview();
    QueueDraw();
  }

  // reset the saturation.
  if (preview_scope_view_.IsValid())
  {
    preview_scope_view_->SetResultsPreviewAnimationValue(0.0);
    preview_scope_view_->ForceCategoryExpansion(stored_activated_unique_id_, false);
    preview_scope_view_->EnableResultTextures(false);
    preview_scope_view_->PopFilterExpansion();
  }
  preview_scope_view_.Release();
  content_view_->SetPresentRedirectedView(true);
}

void DashView::AboutToShow()
{
  visible_ = true;
  search_bar_->text_entry()->SelectAll();

  /* Give the scopes a chance to prep data before we map them  */
  if (active_scope_view_)
  {
    scope_bar_->Activate(active_scope_view_->scope()->id());

    active_scope_view_->SetVisible(true);
    active_scope_view_->scope()->view_type = ScopeViewType::SCOPE_VIEW;

    // this will make sure the spinner animates if the search takes a while
    search_bar_->ForceLiveSearch();
  }

  // if a preview is open, close it
  if (preview_displaying_)
  {
    ClosePreview();
  }

  overlay_window_buttons_->Show();

  renderer_.UpdateBlurBackgroundSize(content_geo_, GetRenderAbsoluteGeometry(), false);
  renderer_.AboutToShow();
}

void DashView::AboutToHide()
{
  if (BackgroundEffectHelper::blur_type == BLUR_STATIC)
  { 
    content_geo_ = {0, 0, 0, 0};
    renderer_.UpdateBlurBackgroundSize(content_geo_, GetRenderAbsoluteGeometry(), false);
  }

  visible_ = false;
  renderer_.AboutToHide();

  if (scopes_)
  {
    for (auto scope : scopes_->GetScopes())
    {
      scope->view_type = ScopeViewType::HIDDEN;
      LOG_DEBUG(logger) << "Setting ViewType " << ScopeViewType::HIDDEN
                            << " on '" << scope->id() << "'";
    }
  }

  if (active_scope_view_.IsValid())
    active_scope_view_->SetVisible(false);

  // if a preview is open, close it
  if (preview_displaying_)
  {
    ClosePreview();
  }

  overlay_window_buttons_->Hide();
}

void DashView::SetupViews()
{
  dash::Style& style = dash::Style::Instance();
  panel::Style &panel_style = panel::Style::Instance();
  int panel_height = panel_style.PanelHeight();

  layout_ = new nux::VLayout();
  layout_->SetLeftAndRightPadding(style.GetVSeparatorSize(), 0);
  layout_->SetTopAndBottomPadding(style.GetHSeparatorSize(), 0);
  SetLayout(layout_);
  layout_->AddLayout(new nux::SpaceLayout(0, 0, panel_height, panel_height), 0);

  content_layout_ = new DashLayout(NUX_TRACKER_LOCATION);
  content_layout_->SetTopAndBottomPadding(style.GetDashViewTopPadding(), 0);

  content_view_ = new DashContentView(NUX_TRACKER_LOCATION);
  content_view_->SetLayout(content_layout_);
  layout_->AddView(content_view_, 1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);

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
  search_bar_->showing_filters.changed.connect([this] (bool showing)
  {
    if (active_scope_view_)
    {
      active_scope_view_->filters_expanded = showing;
      QueueDraw();
    }
  });
  search_bar_layout_->AddView(search_bar_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  content_layout_->SetSpecialArea(search_bar_->show_filters());

  scopes_layout_ = new nux::VLayout();
  content_layout_->AddLayout(scopes_layout_, 1, nux::MINOR_POSITION_START);

  scope_bar_ = new ScopeBar();
  AddChild(scope_bar_);
  scope_bar_->scope_activated.connect(sigc::mem_fun(this, &DashView::OnScopeBarActivated));
  content_layout_->AddView(scope_bar_, 0, nux::MINOR_POSITION_CENTER);
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
  nux::Geometry const& geo = GetGeometry();
  content_geo_ = GetBestFitGeometry(geo);
  dash::Style& style = dash::Style::Instance();

  // kinda hacky, but it makes sure the content isn't so big that it throws
  // the bottom of the dash off the screen
  // not hugely happy with this, so FIXME
  scopes_layout_->SetMaximumHeight (std::max(0, content_geo_.height - search_bar_->GetGeometry().height - scope_bar_->GetGeometry().height - style.GetDashViewTopPadding()));
  scopes_layout_->SetMinimumHeight (std::max(0, content_geo_.height - search_bar_->GetGeometry().height - scope_bar_->GetGeometry().height - style.GetDashViewTopPadding()));

  layout_->SetMinMaxSize(content_geo_.width, content_geo_.y + content_geo_.height);

  // Minus the padding that gets added to the left
  float tile_width = style.GetTileWidth();
  style.SetDefaultNColumns(floorf((content_geo_.width - 32) / tile_width));

  ubus_manager_.SendMessage(UBUS_DASH_SIZE_CHANGED, g_variant_new("(ii)", content_geo_.width, content_geo_.height));

  if (preview_displaying_)
    preview_container_->SetGeometry(layout_->GetGeometry());

  renderer_.UpdateBlurBackgroundSize(content_geo_, GetRenderAbsoluteGeometry(), false);

  QueueDraw();
}

// Gives us the width and height of the contents that will give us the best "fit",
// which means that the icons/views will not have unnecessary padding, everything will
// look tight
nux::Geometry DashView::GetBestFitGeometry(nux::Geometry const& for_geo)
{
  dash::Style& style = dash::Style::Instance();
  panel::Style &panel_style = panel::Style::Instance();
  int panel_height = panel_style.PanelHeight();

  int width = 0, height = 0;
  int tile_width = style.GetTileWidth();
  int tile_height = style.GetTileHeight();
  int category_height = (style.GetPlacesGroupTopSpace() + style.GetCategoryIconSize()  + style.GetPlacesGroupResultTopPadding() + tile_height);
  int half = for_geo.width / 2;

  // if default dash size is bigger than half a screens worth of items, go for that.
  while ((width += tile_width) < half)
    ;

  width = std::max(width, tile_width * DASH_TILE_HORIZONTAL_COUNT);
  width += style.GetVSeparatorSize();
  width += style.GetPlacesGroupResultLeftPadding() + DASH_RESULT_RIGHT_PAD;


  height = style.GetHSeparatorSize();
  height += style.GetDashViewTopPadding();
  height += search_bar_->GetGeometry().height;
  height += category_height * DASH_DEFAULT_CATEGORY_COUNT; // adding three categories
  height += scope_bar_->GetGeometry().height;

  // width/height shouldn't be bigger than the geo available.
  width = std::min(width, for_geo.width); // launcher width is taken into account in for_geo.
  height = std::min(height, for_geo.height - panel_height); // panel height is not taken into account in for_geo.

  if (style.always_maximised)
  {
    width = std::max(0, for_geo.width);
    height = std::max(0, for_geo.height - panel_height);
  }
  return nux::Geometry(0, panel_height, width, height);
}

void DashView::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  panel::Style &panel_style = panel::Style::Instance();
  nux::Geometry const& renderer_geo_abs(GetRenderAbsoluteGeometry());
  nux::Geometry renderer_geo(GetGeometry());
  int panel_height = panel_style.PanelHeight();

  renderer_geo.y += panel_height;
  renderer_geo.height += panel_height;

  renderer_.DrawFull(graphics_engine, content_geo_, renderer_geo_abs, renderer_geo, false);
}

void DashView::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  panel::Style& panel_style = panel::Style::Instance();
  int panel_height = panel_style.PanelHeight();

  nux::Geometry renderer_geo_abs(GetAbsoluteGeometry());
  renderer_geo_abs.y += panel_height;
  renderer_geo_abs.height -= panel_height;

  nux::Geometry renderer_geo(GetGeometry());
  renderer_geo.y += panel_height;
  renderer_geo.height += panel_height;

  renderer_.DrawInner(graphics_engine, content_geo_, renderer_geo_abs, renderer_geo);

  nux::Geometry const& geo_layout(layout_->GetGeometry());

  // See lp bug: 1125346 (The sharp white line between dash and launcher is missing)
  nux::Geometry clip_geo = geo_layout;
  clip_geo.x += 1;
  graphics_engine.PushClippingRectangle(clip_geo);

  if (IsFullRedraw())
  {
    nux::GetPainter().PushBackgroundStack();
  }
  else
  {
    nux::GetPainter().PaintBackground(graphics_engine, geo_layout);
  }

  if (preview_container_.IsValid())
  {
    nux::Geometry geo_split_clip;
    DrawDashSplit(graphics_engine, geo_split_clip);

    graphics_engine.PushClippingRectangle(geo_split_clip);

    if (preview_scope_view_.IsValid())
    {
      DrawPreviewResultTextures(graphics_engine, force_draw);
    }

    DrawPreviewContainer(graphics_engine);

    // preview always on top.
    DrawPreview(graphics_engine, force_draw);

    graphics_engine.PopClippingRectangle();
  }
  else
  {
    layout_->ProcessDraw(graphics_engine, force_draw);
  }

  if (IsFullRedraw())
  {
    nux::GetPainter().PopBackgroundStack();
  }

  graphics_engine.PopClippingRectangle();

  renderer_.DrawInnerCleanup(graphics_engine, content_geo_, renderer_geo_abs, renderer_geo);
}

void DashView::DrawDashSplit(nux::GraphicsEngine& graphics_engine, nux::Geometry& split_clip)
{
  nux::Geometry const& geo_layout(layout_->GetGeometry());
  split_clip = geo_layout;

  if (animate_split_value_ == 1.0)
    return;

  // if we're not presenting, then we must be manually rendering it's backup texture.
  if (!content_view_->PresentRedirectedView() && content_view_->BackupTexture().IsValid())
  {
    unsigned int alpha, src, dest = 0;
    graphics_engine.GetRenderStates().GetBlend(alpha, src, dest);
    graphics_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    nux::TexCoordXForm texxform;
    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    texxform.FlipVCoord(true);

    // Scope Bar
    texxform.uoffset = (scope_bar_->GetX() - content_view_->GetX())/(float)content_view_->GetWidth();
    texxform.voffset = (scope_bar_->GetY() - content_view_->GetY())/(float)content_view_->GetHeight();

    int start_y = scope_bar_->GetY();
    int final_y = geo_layout.y + geo_layout.height + PREVIEW_ICON_SPLIT_OFFSCREEN_OFFSET;

    int scope_y = (1.0f - animate_split_value_) * start_y + (animate_split_value_ * final_y);

    graphics_engine.QRP_1Tex
    (
      scope_bar_->GetX(),
      scope_y,
      scope_bar_->GetWidth(),
      scope_bar_->GetHeight(),
      content_view_->BackupTexture(),
      texxform,
      nux::Color((1.0-animate_split_value_), (1.0-animate_split_value_), (1.0-animate_split_value_), (1.0-animate_split_value_))
    );

    split_clip.height = std::min(scope_y, geo_layout.height);

    if (active_scope_view_ && active_scope_view_->GetPushedFilterExpansion())
    {
      // Search Bar
      texxform.uoffset = (search_bar_->GetX() - content_view_->GetX())/(float)content_view_->GetWidth();
      texxform.voffset = (search_bar_->GetY() - content_view_->GetY())/(float)content_view_->GetHeight();

      start_y = search_bar_->GetY();
      final_y = geo_layout.y - search_bar_->GetHeight() - PREVIEW_ICON_SPLIT_OFFSCREEN_OFFSET;

      graphics_engine.QRP_1Tex
      (
        search_bar_->GetX(),
        (1.0f - animate_split_value_) * start_y + (animate_split_value_ * final_y),
        search_bar_->GetWidth() - active_scope_view_->filter_bar()->GetWidth(),
        search_bar_->GetHeight(),
        content_view_->BackupTexture(),
        texxform,
        nux::Color((1.0-animate_split_value_), (1.0-animate_split_value_), (1.0-animate_split_value_), (1.0-animate_split_value_))
      );

      // Filter Bar
      texxform.uoffset = (active_scope_view_->filter_bar()->GetX() -content_view_->GetX())/(float)content_view_->GetWidth();
      texxform.voffset = (search_bar_->GetY() - content_view_->GetY())/(float)content_view_->GetHeight();

      int start_x = active_scope_view_->filter_bar()->GetX();
      int final_x = content_view_->GetX() + content_view_->GetWidth() + PREVIEW_ICON_SPLIT_OFFSCREEN_OFFSET;

      int filter_x = (1.0f - animate_split_value_) * start_x + (animate_split_value_ * final_x);

      graphics_engine.QRP_1Tex
      (
        filter_x,
        search_bar_->GetY(),
        active_scope_view_->filter_bar()->GetWidth(),
        active_scope_view_->filter_bar()->GetY() + active_scope_view_->filter_bar()->GetHeight(),
        content_view_->BackupTexture(),
        texxform,
        nux::Color((1.0-animate_split_value_), (1.0-animate_split_value_), (1.0-animate_split_value_), (1.0-animate_split_value_))
      );

      split_clip.width = filter_x;
    }
    else
    {
      // Search Bar
      texxform.uoffset = (search_bar_->GetX() - content_view_->GetX())/(float)content_view_->GetWidth();
      texxform.voffset = (search_bar_->GetY() - content_view_->GetY())/(float)content_view_->GetHeight();

      int start_y = search_bar_->GetY();
      int final_y = geo_layout.y - search_bar_->GetHeight() - PREVIEW_ICON_SPLIT_OFFSCREEN_OFFSET;

      graphics_engine.QRP_1Tex
      (
        search_bar_->GetX(),
        (1.0f - animate_split_value_) * start_y + (animate_split_value_ * final_y),
        search_bar_->GetWidth(),
        search_bar_->GetHeight(),
        content_view_->BackupTexture(),
        texxform,
        nux::Color((1.0-animate_split_value_), (1.0-animate_split_value_), (1.0-animate_split_value_), (1.0-animate_split_value_))
      );
    }

    graphics_engine.GetRenderStates().SetBlend(alpha, src, dest);
  }
}

void DashView::DrawPreviewContainer(nux::GraphicsEngine& graphics_engine)
{
  if (animate_preview_container_value_ == 0.0f)
    return;

  nux::Geometry const& geo_content = content_view_->GetGeometry();
  nux::Geometry geo_abs = GetAbsoluteGeometry();
  nux::Geometry geo_abs_preview = preview_container_->GetLayoutGeometry();

  unsigned int alpha, src, dest = 0;
  graphics_engine.GetRenderStates().GetBlend(alpha, src, dest);
  graphics_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  // Triangle pointed at preview item
  if (opening_column_x_ != -1)
  {
    int final_width = 14;
    int final_height = 12;

    int x_center = geo_content.x + (opening_column_x_ - geo_abs.x) + opening_column_width_ / 2;
    int start_y = geo_abs_preview.y - geo_abs.y;

    int x1 = x_center - final_width/2;
    int final_y1 = start_y;

    int x2 = x_center + final_width/2;
    int final_y2 = start_y;

    int x3 = x_center;
    int final_y3 = start_y - final_height;

    graphics_engine.QRP_Triangle
    (
      x1,
      (1.0f-animate_preview_container_value_) * start_y + (animate_preview_container_value_ * final_y1),

      x2,
      (1.0f-animate_preview_container_value_) * start_y + (animate_preview_container_value_ * final_y2),

      x3,
      (1.0f-animate_preview_container_value_) * start_y + (animate_preview_container_value_ * final_y3),

      nux::Color(0.0f, 0.0f, 0.0f, 0.10f)
    );
  }

  // Preview Background
  int start_width = geo_content.width;
  int start_x = geo_content.x;
  int start_y = geo_abs_preview.y - geo_abs.y;

  int final_x = geo_content.x;
  int final_y = start_y;
  int final_width = geo_content.width;
  int final_height = geo_abs_preview.height;

  graphics_engine.QRP_Color
  (
    (1.0f-animate_preview_container_value_) * start_x + (animate_preview_container_value_ * final_x),
    (1.0f-animate_preview_container_value_) * start_y + (animate_preview_container_value_ * final_y),
    (1.0f-animate_preview_container_value_) * start_width + (animate_preview_container_value_ * final_width),
    (animate_preview_container_value_ * final_height),
    nux::Color(0.0f, 0.0f, 0.0f, 0.10f)
  );

  graphics_engine.GetRenderStates().SetBlend(alpha, src, dest);
}

void DashView::DrawPreviewResultTextures(nux::GraphicsEngine& graphics_engine, bool /*force_draw*/)
{
  nux::Geometry const& geo_layout = layout_->GetGeometry();
  nux::Geometry geo_abs = GetAbsoluteGeometry();
  nux::Geometry geo_abs_preview = preview_container_->GetLayoutGeometry();

  unsigned int alpha, src, dest = 0;
  graphics_engine.GetRenderStates().GetBlend(alpha, src, dest);
  graphics_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  nux::TexCoordXForm texxform;
  texxform.FlipVCoord(true);
  texxform.uoffset = 0.0f;
  texxform.voffset = 0.0f;
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

  std::vector<ResultViewTexture::Ptr> result_textures = preview_scope_view_->GetResultTextureContainers();
  std::vector<ResultViewTexture::Ptr> top_textures;

  int height_concat_below = 0;

  // int i = 0;
  for (auto it = result_textures.begin(); it != result_textures.end(); ++it)
  {
    ResultViewTexture::Ptr const& result_texture = *it;
    if (!result_texture)
      continue;

    // split above.
    if (result_texture->abs_geo.y <= opening_row_y_)
    {
      top_textures.push_back(result_texture);
      continue;
    }

    // Bottom textures.
    int start_x = result_texture->abs_geo.x - geo_abs.x;
    int final_x = start_x;

    int start_y = result_texture->abs_geo.y - geo_abs.y;
    int final_y = geo_abs_preview.y + geo_abs_preview.height - geo_abs.y;
    final_y += std::min(60, ((geo_layout.y + geo_layout.height) - final_y)/4);
    final_y += height_concat_below;

    nux::Geometry geo_tex_top((1.0f-animate_split_value_) * start_x + (animate_split_value_ * final_x),
                       (1.0f-animate_split_value_) * start_y + (animate_split_value_ * final_y),
                       result_texture->abs_geo.width,
                       result_texture->abs_geo.height);

    height_concat_below += geo_tex_top.height;

    // off the bottom
    if (geo_tex_top.y <= geo_layout.y + geo_layout.height)
    {
      preview_scope_view_->RenderResultTexture(result_texture);
      // If we haven't got it now, we're not going to get it
      if (!result_texture->texture.IsValid())
        continue;

      graphics_engine.QRP_1Tex
      (
        geo_tex_top.x,
        geo_tex_top.y,
        geo_tex_top.width,
        geo_tex_top.height,
        result_texture->texture,
        texxform,
        nux::Color(1.0f, 1.0f, 1.0f, 1.0f)
      );
    }
  }

  // Top Textures (in reverse)
  int height_concat_above = 0;
  for (auto it = top_textures.rbegin(); it != top_textures.rend(); ++it)
  {
    ResultViewTexture::Ptr const& result_texture = *it;

    // each texture starts at a higher position.
    height_concat_above += result_texture->abs_geo.height;

    int start_x = result_texture->abs_geo.x - geo_abs.x;
    int final_x = start_x;

    int start_y = result_texture->abs_geo.y - geo_abs.y;
    int final_y = -height_concat_above + (geo_abs_preview.y - geo_abs.y);

    nux::Geometry geo_tex_top((1.0f-animate_split_value_) * start_x + (animate_split_value_ * final_x),
                       (1.0f-animate_split_value_) * start_y + (animate_split_value_ * final_y),
                       result_texture->abs_geo.width,
                       result_texture->abs_geo.height);

    // off the top
    if (geo_tex_top.y + geo_tex_top.height >= geo_layout.y)
    {
      preview_scope_view_->RenderResultTexture(result_texture);
      // If we haven't got it now, we're not going to get it
      if (!result_texture->texture.IsValid())
        continue;

      graphics_engine.QRP_1Tex
      (
        geo_tex_top.x,
        geo_tex_top.y,
        geo_tex_top.width,
        geo_tex_top.height,
        result_texture->texture,
        texxform,
        nux::Color(1.0f, 1.0f, 1.0f, 1.0f)
      );
    }
  }

  graphics_engine.GetRenderStates().SetBlend(alpha, src, dest);
}

void DashView::DrawPreview(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  if (animate_preview_value_ > 0.0f)
  {
    bool animating = animate_split_value_ != 1.0f || animate_preview_value_ < 1.0f;
    bool preview_force_draw = force_draw || animating || IsFullRedraw();

    if (preview_force_draw)
      nux::GetPainter().PushBackgroundStack();

    if (animate_preview_value_ < 1.0f && preview_container_->RedirectRenderingToTexture())
    {
      preview_container_->SetPresentRedirectedView(false);
      preview_container_->ProcessDraw(graphics_engine, preview_force_draw);

      unsigned int alpha, src, dest = 0;
      graphics_engine.GetRenderStates().GetBlend(alpha, src, dest);
      graphics_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

      nux::ObjectPtr<nux::IOpenGLBaseTexture> preview_texture = preview_container_->BackupTexture();
      if (preview_texture)
      {
        nux::TexCoordXForm texxform;
        texxform.FlipVCoord(true);
        texxform.uoffset = 0.0f;
        texxform.voffset = 0.0f;
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

        nux::Geometry const& geo_preview = preview_container_->GetGeometry();
        graphics_engine.QRP_1Tex
        (
          geo_preview.x,
          geo_preview.y,
          geo_preview.width,
          geo_preview.height,
          preview_texture,
          texxform,
          nux::Color(animate_preview_value_, animate_preview_value_, animate_preview_value_, animate_preview_value_)
        );
      }

      preview_container_->SetPresentRedirectedView(true);

      graphics_engine.GetRenderStates().SetBlend(alpha, src, dest);
    }
    else
    {
      preview_container_->ProcessDraw(graphics_engine, preview_force_draw);
    }

    if (preview_force_draw)
      nux::GetPainter().PopBackgroundStack();
  }
}

void DashView::OnMouseButtonDown(int x, int y, unsigned long button, unsigned long key)
{
  dash::Style& style = dash::Style::Instance();
  nux::Geometry geo(content_geo_);

  if (Settings::Instance().form_factor() == FormFactor::DESKTOP)
  {
    geo.width += style.GetDashRightTileWidth();
    geo.height += style.GetDashBottomTileHeight();
  }
}

void DashView::OnActivateRequest(GVariant* args)
{
  glib::String uri;
  glib::String search_string;
  ScopeHandledType handled_type;

  g_variant_get(args, "(sus)", &uri, &handled_type, &search_string);

  std::string id(AnalyseScopeURI(uri.Str()));

  LOG_DEBUG(logger) << "External activation request: " << id << " (uri: "<< uri.Str() << ")";

  // we got an activation request, we should probably close the preview
  if (preview_displaying_)
  {
    ClosePreview();
  }

  if (visible_ && handled_type == ScopeHandledType::NOT_HANDLED)
  {
    ubus_manager_.SendMessage(UBUS_OVERLAY_CLOSE_REQUEST, NULL,
                              glib::Source::Priority::HIGH);
  }
  else if (!visible_ || handled_type == ScopeHandledType::GOTO_DASH_URI)
  {
    if (!scopes_->GetScope(id))
    {
      // should trigger the addition of the scope.
      scopes_->AppendScope(id);
    }
    scope_bar_->Activate(id);

    if (!visible_)
      ubus_manager_.SendMessage(UBUS_DASH_EXTERNAL_ACTIVATION);
  }
}

std::string DashView::AnalyseScopeURI(std::string const& uri)
{
  impl::ScopeFilter filter = impl::parse_scope_uri(uri);

  if (!filter.filters.empty())
  {
    scope_views_[filter.id]->filters_expanded = true;
    // update the scope for each filter
    for (auto p : filter.filters) {
      UpdateScopeFilter(filter.id, p.first, p.second);
    }
  }

  return filter.id;
}

void DashView::UpdateScopeFilter(std::string scope_id, std::string filter_name, std::string value)
{
  if (scopes_ && scopes_->GetScope(scope_id))
  {
    Scope::Ptr scope = scopes_->GetScope(scope_id);

    Filters::Ptr filters = scope->filters;

    for (unsigned int i = 0; i < filters->count(); ++i)
    {
      Filter::Ptr filter = filters->FilterAtIndex(i);

      if (filter->id() == filter_name)
      {
        UpdateScopeFilterValue(filter, value);
      }
    }
  }
}

void DashView::UpdateScopeFilterValue(Filter::Ptr filter, std::string value)
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
  search_in_progress_ = true;
}

void DashView::OnLiveSearchReached(std::string const& search_string)
{
  // reset and set it again once we're sure a search is happening
  search_in_progress_ = false;

  LOG_DEBUG(logger) << "Live search reached: " << search_string;
  if (active_scope_view_)
  {
    if (active_scope_view_->PerformSearch(search_string, sigc::mem_fun(this, &DashView::OnScopeSearchFinished)))
    {
      search_in_progress_ = true;
    }
  }
}

void DashView::OnScopeSearchFinished(std::string const& scope_id, std::string const& search_string, glib::Error const& err)
{
  // match active scope?
  auto scope_pos = scope_views_.find(scope_id);
  if (scope_pos == scope_views_.end() || scope_pos->second != active_scope_view_)
    return;

  if (search_string == search_bar_->search_string)
  {
    if (err)
      LOG_WARNING(logger) << "Search failed  '"<< search_string <<"'=> " << err;
    else
      LOG_DEBUG(logger) << "Search completed: " << search_string;

    search_bar_->SetSearchFinished();
    search_in_progress_ = false;

    activate_timeout_.reset();
    if (activate_on_finish_ && !err)
      OnEntryActivated();
    activate_on_finish_= false;
  }
}

void DashView::OnScopeAdded(Scope::Ptr const& scope, int position)
{
  LOG_DEBUG(logger) << "Scope Added: " << scope->id();

  scope_bar_->AddScope(scope);

  nux::ObjectPtr<ScopeView> view(new ScopeView(scope, search_bar_->show_filters()));
  AddChild(view.GetPointer());
  view->SetVisible(false);
  view->result_activated.connect(sigc::mem_fun(this, &DashView::OnResultActivated));

  scopes_layout_->AddView(view.GetPointer(), 1);
  scope_views_[scope->id] = view;

  // set form factor used for the searches
  scope->form_factor = "desktop";
  scope->activated.connect(sigc::mem_fun(this, &DashView::OnResultActivatedReply));
  scope->connected.changed.connect([this] (bool value) { });

  // Hook up to the new preview infrastructure
  scope->preview_ready.connect([this] (LocalResult const& result, Preview::Ptr model)
  {
    // HACK: Atm we don't support well the fact that a preview can be sent from
    // an ActionResponse and therefore transition does not work, this hack allows
    // to set the navigation mode to ensure that we have a nice transition
    if (dynamic_cast<PaymentPreview*>(model.get()) != NULL)
    {
      preview_state_machine_.left_results.Set(0);
      preview_state_machine_.right_results.Set(0);
      preview_navigation_mode_ = previews::Navigation::RIGHT;
    }
    preview_state_machine_.ActivatePreview(model); // this does not immediately display a preview - we now wait.
  });
}

void DashView::OnScopeBarActivated(std::string const& id)
{
  if (scope_views_.find(id) == scope_views_.end())
  {
    LOG_WARN(logger) << "Unable to find Scope " << id;
    return;
  }

  if (IsCommandLensOpen() && scope_bar_->IsVisible())
  {
    scope_bar_->SetVisible(false);
  }
  else if (!scope_bar_->IsVisible())
  {
    scope_bar_->SetVisible(true);
  }

  if (active_scope_view_.IsValid())
    active_scope_view_->SetVisible(false);

  nux::ObjectPtr<ScopeView> view = active_scope_view_ = scope_views_[id];

  view->SetVisible(true);
  view->AboutToShow();

  for (auto it: scope_views_)
  {
    bool id_matches = it.first == id;
    ScopeViewType view_type = id_matches ? ScopeViewType::SCOPE_VIEW : ScopeViewType::HIDDEN;
    it.second->SetVisible(id_matches);
    it.second->view_type = view_type;

    LOG_DEBUG(logger) << "Setting ViewType " << view_type
                      << " on '" << it.first << "'";
  }

  search_bar_->SetVisible(true);
  QueueRelayout();
  search_bar_->search_string = view->search_string;
  search_bar_->search_hint = view->scope()->search_hint;
  search_bar_->showing_filters = view->filters_expanded();
  search_bar_->ForceLiveSearch();

  search_bar_->text_entry()->SelectAll();
  search_bar_->can_refine_search = view->can_refine_search();
  scope_can_refine_connection_ = view->can_refine_search.changed.connect([this] (bool can_refine_search) {
    search_bar_->can_refine_search = can_refine_search;
  });

  // Fix for a nux quirk. Unparented objects cannot have focus because there will be a break in the
  // parent tree and the compositor will not be able to walk the focus tree. This still needs to be here
  // for when scopes switch via the scope bar and the focus nees to move away from the scope bar to the search bar..
  if (GetParentObject())
    nux::GetWindowCompositor().SetKeyFocusArea(default_focus());

  view->QueueDraw();
  QueueDraw();
}

void DashView::OnResultActivatedReply(LocalResult const& local_result, ScopeHandledType type, glib::HintsMap const& hints)
{
  // We don't want to close the dash if there was another activation pending
  if (type == NOT_HANDLED)
  {
    if (!DoFallbackActivation(local_result.uri))
      return;
  }
  else if (type == SHOW_DASH)
  {
    return;
  }
  else if (type == PERFORM_SEARCH)
  {
    auto it = hints.find("query");
    if (it != hints.end())
    {
      search_bar_->search_string = it->second.GetString();
      return;
    }
  }

  ubus_manager_.SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);
}

bool DashView::DoFallbackActivation(std::string const& uri)
{
  if (g_str_has_prefix(uri.c_str(), "application://"))
  {
    std::string const& appname = uri.substr(14);
    return application_starter_->Launch(appname, last_activated_timestamp_);
  }
  else if (g_str_has_prefix(uri.c_str(), "unity-runner://"))
  {
    std::string const& appname = uri.substr(15);
    return application_starter_->Launch(appname, last_activated_timestamp_);
  }
  else
    return gtk_show_uri(NULL, uri.c_str(), last_activated_timestamp_, NULL);

  return false;
}

void DashView::DisableBlur()
{
  renderer_.DisableBlur();
}
void DashView::OnEntryActivated()
{
  if (active_scope_view_.IsValid() && !search_in_progress_)
  {
    active_scope_view_->ActivateFirst();
  }
  // delay the activation until we get the SearchFinished signal
  activate_on_finish_ = search_in_progress_;

  if (activate_on_finish_)
  {
    activate_timeout_.reset(new glib::Timeout(MAX_ENTRY_ACTIVATE_WAIT_TIMEOUT, [this] {
      activate_on_finish_ = false;
      active_scope_view_->ActivateFirst();
      return FALSE;
    }));
  }
}

// Keyboard navigation
bool DashView::AcceptKeyNavFocus()
{
  return false;
}

std::string const DashView::GetIdForShortcutActivation(std::string const& shortcut) const
{
  Scope::Ptr scope = scopes_ ? scopes_->GetScopeForShortcut(shortcut) : Scope::Ptr();
  if (scope)
    return scope->id;
  return "";
}

std::vector<char> DashView::GetAllShortcuts()
{
  std::vector<char> result;

  if (scopes_)
  {
    for (Scope::Ptr scope: scopes_->GetScopes())
    {
      std::string shortcut = scope->shortcut;
      if(shortcut.size() > 0)
        result.push_back(shortcut.at(0));
    }
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
    else if (!search_bar_->search_string().empty())
      search_bar_->search_string = "";
    else
      ubus_manager_.SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);

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

void DashView::AddProperties(debug::IntrospectionData& introspection)
{
  dash::Style& style = dash::Style::Instance();
  int num_rows = 1; // The search bar
  std::vector<bool> button_on_monitor;

  if (active_scope_view_.IsValid())
    num_rows += active_scope_view_->GetNumRows();

  std::string form_factor("unknown");

  if (Settings::Instance().form_factor() == FormFactor::NETBOOK)
    form_factor = "netbook";
  else if (Settings::Instance().form_factor() == FormFactor::DESKTOP)
    form_factor = "desktop";
  else if (Settings::Instance().form_factor() == FormFactor::TV)
    form_factor = "tv";

  for (unsigned i = 0; i < monitors::MAX; ++i)
    button_on_monitor.push_back(overlay_window_buttons_->IsVisibleOnMonitor(i));

  introspection.add(nux::Geometry(GetAbsoluteX(), GetAbsoluteY(), content_geo_.width, content_geo_.height))
               .add("num_rows", num_rows)
               .add("form_factor", form_factor)
               .add("right-border-width", style.GetDashRightTileWidth())
               .add("bottom-border-height", style.GetDashBottomTileHeight())
               .add("preview_displaying", preview_displaying_)
               .add("preview_animation", animate_split_value_ * animate_preview_container_value_ * animate_preview_value_)
               .add("dash_maximized", style.always_maximised())
               .add("overlay_window_buttons_shown", glib::Variant::FromVector(button_on_monitor));
}

nux::Area* DashView::KeyNavIteration(nux::KeyNavDirection direction)
{
  if (preview_displaying_)
  {
    return preview_container_->KeyNavIteration(direction);
  }
  else if (direction == nux::KEY_NAV_DOWN && search_bar_ && active_scope_view_.IsValid())
  {
    auto show_filters = search_bar_->show_filters();
    auto fscroll_view = active_scope_view_->fscroll_view();

    if (show_filters && show_filters->HasKeyFocus())
    {
      if (fscroll_view->IsVisible() && fscroll_view)
        return fscroll_view->KeyNavIteration(direction);
      else
        return active_scope_view_->KeyNavIteration(direction);
    }
  }
  return this;
}

void DashView::ProcessDndEnter()
{
  auto const& event = nux::GetGraphicsDisplay()->GetCurrentEvent();

  // Don't close the dash if the mouse is over the vertical line between the dash and the launcher.
  if (event.x != GetAbsoluteX())
    ubus_manager_.SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);
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
  default:
    auto const& close_key = WindowManager::Default().close_window_key();

    if (close_key.first == special_keys_state && close_key.second == x11_key_code)
    {
      ubus_manager_.SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);
      return nullptr;
    }

    direction = KEY_NAV_NONE;
  }

  if (preview_displaying_)
  {
    return preview_container_->FindKeyFocusArea(key_symbol, x11_key_code, special_keys_state);
  }

  // We should not do it here, but I really don't want to make DashView
  // focusable and I'm not able to know if ctrl is pressed in
  // DashView::KeyNavIteration.
   nux::InputArea* focus_area = nux::GetWindowCompositor().GetKeyFocusArea();

  if (direction != KEY_NAV_NONE && key_symbol == nux::NUX_KEYDOWN && !search_bar_->im_preedit)
  {
    std::list<nux::Area*> tabs;
    if (active_scope_view_.IsValid())
    {
      for (auto category : active_scope_view_->GetOrderedCategoryViews())
      {
        if (category->IsVisible())
          tabs.push_back(category.GetPointer());
      }
    }

    if (search_bar_ && search_bar_->show_filters() &&
        search_bar_->show_filters()->IsVisible())
    {
      tabs.push_back(search_bar_->show_filters());
    }

    if (active_scope_view_.IsValid() &&
        active_scope_view_->filter_bar() && active_scope_view_->fscroll_view() &&
        active_scope_view_->fscroll_view()->IsVisible())
    {
      for (auto child : active_scope_view_->filter_bar()->GetLayout()->GetChildren())
      {
        FilterExpanderLabel* filter = dynamic_cast<FilterExpanderLabel*>(child);
        if (filter)
          tabs.push_back(filter->expander_view());
      }
    }

    if (direction == KEY_NAV_TAB_PREVIOUS)
    {
      if (ctrl)
      {
        scope_bar_->ActivatePrevious();
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
        scope_bar_->ActivateNext();
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
    if (keyboard::is_printable_key_symbol(x11_key_code) ||
        keyboard::is_move_key_symbol(x11_key_code))
    {
      search_key = true;
    }
  }

  if (!preview_displaying_ && (search_key || search_bar_->im_preedit))
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

  if (overlay_window_buttons_->GetGeometry().IsInside(mouse_position))
  {
    return overlay_window_buttons_->FindAreaUnderMouse(mouse_position, event_type);
  }
  else if (preview_displaying_)
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

nux::Geometry const& DashView::GetContentGeometry() const
{
  return content_geo_;
}

nux::Geometry DashView::GetRenderAbsoluteGeometry() const
{
  panel::Style &panel_style = panel::Style::Instance();
  int panel_height = panel_style.PanelHeight();

  nux::Geometry renderer_geo_abs(GetAbsoluteGeometry());
  renderer_geo_abs.y += panel_height;
  renderer_geo_abs.height -= panel_height;
  return renderer_geo_abs;
}

}
}
