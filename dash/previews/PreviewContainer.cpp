// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#include "PreviewContainer.h"
#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>

#include "unity-shared/IntrospectableWrappers.h"
#include "unity-shared/TimeUtil.h"
#include "unity-shared/PreviewStyle.h"
#include "unity-shared/GraphicsUtils.h"
#include "PreviewNavigator.h"
#include <boost/math/constants/constants.hpp>
#include "config.h"

namespace unity
{
namespace dash
{
namespace previews
{
DECLARE_LOGGER(logger, "unity.dash.preview.container");

Navigation operator&(const Navigation lhs, const Navigation rhs)
{
  return Navigation(int(lhs) & int(rhs));
}

namespace
{
const int ANIM_DURATION_LONG = 500;
const int PREVIEW_SPINNER_WAIT = 2000;

const std::string ANIMATION_IDLE = "animation-idle";
}

class PreviewContent : public nux::Layout, public debug::Introspectable
{
public:
  PreviewContent(PreviewContainer*const parent)
  : parent_(parent)
  , progress_(0.0)
  , curve_progress_(0.0)
  , animating_(false)
  , waiting_preview_(false)
  , rotation_(0.0)
  , preview_initiate_count_(0)
  , nav_complete_(0)
  , relative_nav_index_(0)
  {
    geometry_changed.connect([this](nux::Area*, nux::Geometry& geo)
    {
      // Need to update the preview geometries when updating the container geo.
      UpdateAnimationProgress(progress_, curve_progress_);
    });
    Style& style = previews::Style::Instance();

    spin_= style.GetSearchSpinIcon(32);
  }

  // From debug::Introspectable
  std::string GetName() const
  {
    return "PreviewContent";
  }

  void AddProperties(debug::IntrospectionData& introspection)
  {
    introspection
      .add("animating", animating_)
      .add("animation_progress", progress_)
      .add("waiting_preview", waiting_preview_)
      .add("preview-initiate-count", preview_initiate_count_)
      .add("navigation-complete-count", nav_complete_)
      .add("relative-nav-index", relative_nav_index_);
  }

  void PushPreview(previews::Preview::Ptr preview, Navigation direction)
  {
    if (preview)
    {
      preview_initiate_count_++;
      StopPreviewWait();
      // the parents layout will not change based on the previews.
      preview->SetReconfigureParentLayoutOnGeometryChange(false);
      
      AddChild(preview.GetPointer());
      AddView(preview.GetPointer());
      preview->SetVisible(false);
    }
    else
    {
      // if we push a null preview, then start waiting.
      StartPreviewWait();      
    }
    PreviewSwipe swipe;
    swipe.direction = direction;
    swipe.preview = preview;
    push_preview_.push(swipe);

    if (!animating_)
    {
      UpdateAnimationProgress(0.0, 0.0);
    }
    start_navigation.emit();
  }

  bool IsAnimating() { return animating_; }

  int SwipeQueueSize() const { return push_preview_.size(); }

  float GetAnimationProgress() const { return progress_; }

  void UpdateAnimationProgress(float progress, float curve_progress)
  {
    progress_ = progress;
    curve_progress_ = curve_progress;

    if (!animating_)
    {
      if (!push_preview_.empty())
      {
        animating_= true;
        swipe_ = push_preview_.front();
        push_preview_.pop();

        if (current_preview_)
          current_preview_->OnNavigateOut();
        if (swipe_.preview)
          swipe_.preview->OnNavigateIn();
      }
    }

    nux::Geometry const& geometry = GetGeometry();
    if (animating_)
    { 
       // swipe out.
      if (current_preview_)
      {
        current_preview_->SetVisible(true);

        nux::Geometry swipeOut = geometry;
        if (swipe_.direction == Navigation::RIGHT)
          swipeOut.OffsetPosition(-(curve_progress * (parent_->GetGeometry().width - geometry.x)), 0);
        else if (swipe_.direction == Navigation::LEFT)
          swipeOut.OffsetPosition(curve_progress* (parent_->GetGeometry().width - geometry.x), 0);
        current_preview_->SetGeometry(swipeOut);
      }
 
       // swipe in
      if (swipe_.preview)
      {
        swipe_.preview->SetVisible(true);

        nux::Geometry swipeIn = geometry;
        if (swipe_.direction == Navigation::RIGHT)
          swipeIn.OffsetPosition(float(parent_->GetGeometry().width - geometry.x) - (curve_progress * (parent_->GetGeometry().width - geometry.x)), 0);
        else if (swipe_.direction == Navigation::LEFT)
          swipeIn.OffsetPosition(-((1.0-curve_progress)*(parent_->GetGeometry().width - geometry.x)), 0);
        swipe_.preview->SetGeometry(swipeIn);
      }
    }

    if (progress >= 1.0)
    {
      // if we were animating, we need to remove the old preview, and replace it with the new.
      if (animating_)
      {
        animating_ = false;
        if (current_preview_)
        {
          RemoveChild(current_preview_.GetPointer());
          RemoveChildObject(current_preview_.GetPointer());
          current_preview_.Release();
        }
        if (swipe_.preview)
        {
          if (swipe_.direction == Navigation::RIGHT)
            relative_nav_index_++;
          else if (swipe_.direction == Navigation::LEFT)
            relative_nav_index_--;

          current_preview_ = swipe_.preview;
          swipe_.preview.Release();
          if (current_preview_)
            current_preview_->OnNavigateInComplete();
        } 

        // another swipe?
        if (!push_preview_.empty())
        {
          progress_ = 0;
          continue_navigation.emit();
        }
        else
        {
          end_navigation.emit();
        }
      }

      // set the geometry to the whole layout.
      if (current_preview_)
      {
        current_preview_->SetGeometry(geometry);
      }
      nav_complete_++;
    }
  }

  void StartPreviewWait()
  {
    preview_wait_timer_.reset(new glib::Timeout(PREVIEW_SPINNER_WAIT, [this] () {

      if (waiting_preview_)
        return false;

      waiting_preview_ = true;

      rotate_matrix_.Rotate_z(0.0f);
      rotation_ = 0.0f;
      parent_->QueueDraw();
      return false;
    }));
  }

  void StopPreviewWait()
  {
    preview_wait_timer_.reset();
    waiting_preview_ = false;
    parent_->QueueDraw();
  }

  bool OnFrameTimeout()
  {
    frame_timeout_.reset();
    rotation_ += 0.1f;

    if (rotation_ >= 360.0f)
      rotation_ = 0.0f;

    rotate_matrix_.Rotate_z(rotation_);
    parent_->QueueDraw();

    return false;
  }

  // Dont draw in process draw. this is so we can control the z order.
  void ProcessDraw(nux::GraphicsEngine& gfx_engine, bool force_draw)
  {
  }

  void ProcessDraw2(nux::GraphicsEngine& gfx_engine, bool force_draw)
  {
    if (swipe_.preview && swipe_.preview->IsVisible()) { swipe_.preview->ProcessDraw(gfx_engine, force_draw); }
    if (current_preview_ && current_preview_->IsVisible()) { current_preview_->ProcessDraw(gfx_engine, force_draw); }

    if (waiting_preview_)
    { 
      nux::Geometry const& base = GetGeometry(); 

      unsigned int alpha, src, dest = 0;
      gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
      gfx_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

      nux::TexCoordXForm texxform;
      texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
      texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
      texxform.min_filter = nux::TEXFILTER_LINEAR;
      texxform.mag_filter = nux::TEXFILTER_LINEAR;

      nux::Geometry spin_geo(base.x + ((base.width - spin_->GetWidth()) / 2),
                             base.y + ((base.height - spin_->GetHeight()) / 2),
                             spin_->GetWidth(),
                             spin_->GetHeight());
      // Geometry (== Rect) uses integers which were rounded above,
      // hence an extra 0.5 offset for odd sizes is needed
      // because pure floating point is not being used.
      int spin_offset_w = !(base.width % 2) ? 0 : 1;
      int spin_offset_h = !(base.height % 2) ? 0 : 1;

      // we need to apply the rotation transformation first.
      nux::Matrix4 matrix_texture;
      matrix_texture = nux::Matrix4::TRANSLATE(-spin_geo.x - (spin_geo.width + spin_offset_w) / 2.0f,
                                              -spin_geo.y - (spin_geo.height + spin_offset_h) / 2.0f, 0) * matrix_texture;
      matrix_texture = rotate_matrix_ * matrix_texture;
      matrix_texture = nux::Matrix4::TRANSLATE(spin_geo.x + (spin_geo.width + spin_offset_w) / 2.0f,
                                               spin_geo.y + (spin_geo.height + spin_offset_h) / 2.0f, 0) * matrix_texture;

      gfx_engine.SetModelViewMatrix(gfx_engine.GetModelViewMatrix() * matrix_texture);

      gfx_engine.QRP_1Tex(spin_geo.x,
                          spin_geo.y,
                          spin_geo.width,
                          spin_geo.height,
                          spin_->GetDeviceTexture(),
                          texxform,
                          nux::color::White);

      // revert to model view matrix stack
      gfx_engine.ApplyModelViewMatrix();

      gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);

      if (!frame_timeout_)
      {
        frame_timeout_.reset(new glib::Timeout(22, sigc::mem_fun(this, &PreviewContent::OnFrameTimeout)));
      }
    }

    draw_cmd_queued_ = false;
  }

  nux::Area* KeyNavIteration(nux::KeyNavDirection direction)
  {
    if (swipe_.preview)
      return swipe_.preview->KeyNavIteration(direction);
    else if (current_preview_)
      return current_preview_->KeyNavIteration(direction);

    return NULL;
  }

  nux::Area* FindKeyFocusArea(unsigned int key_symbol,
                                        unsigned long x11_key_code,
                                        unsigned long special_keys_state)
  {
    if (swipe_.preview)
      return swipe_.preview->FindKeyFocusArea(key_symbol, x11_key_code, special_keys_state);
    else if (current_preview_)
      return current_preview_->FindKeyFocusArea(key_symbol, x11_key_code, special_keys_state);

    return nullptr;
  }

  sigc::signal<void> start_navigation;
  sigc::signal<void> continue_navigation;
  sigc::signal<void> end_navigation;

private:
  PreviewContainer*const parent_;

  // Swipe animation
  struct PreviewSwipe
  {
    Navigation direction;
    previews::Preview::Ptr preview;

    void reset() { preview.Release(); }
  };
  previews::Preview::Ptr current_preview_;
  std::queue<PreviewSwipe> push_preview_;
  PreviewSwipe swipe_;

  float progress_;
  float curve_progress_;
  bool animating_;
  // wait animation
  glib::Source::UniquePtr preview_wait_timer_;
  glib::Source::UniquePtr _frame_timeout;
  bool waiting_preview_;
  nux::ObjectPtr<nux::BaseTexture> spin_;

  glib::Source::UniquePtr frame_timeout_;
  nux::Matrix4 rotate_matrix_;
  float rotation_;

  // intropection data.
  int preview_initiate_count_;
  int nav_complete_;
  int relative_nav_index_;
};

NUX_IMPLEMENT_OBJECT_TYPE(PreviewContainer);

PreviewContainer::PreviewContainer(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , preview_layout_(nullptr)
  , nav_disabled_(Navigation::NONE)
  , navigation_progress_speed_(0.0)
  , navigation_count_(0)
{
  SetAcceptKeyNavFocusOnMouseDown(false);
  SetAcceptKeyNavFocusOnMouseEnter(false);

  SetupViews();
  last_progress_time_.tv_sec = 0;
  last_progress_time_.tv_nsec = 0;

  key_down.connect(sigc::mem_fun(this, &PreviewContainer::OnKeyDown));
  mouse_click.connect(sigc::mem_fun(this, &PreviewContainer::OnMouseDown));
}

PreviewContainer::~PreviewContainer()
{
}

void PreviewContainer::Preview(dash::Preview::Ptr preview_model, Navigation direction)
{
  previews::Preview::Ptr preview_view = preview_model ? previews::Preview::PreviewForModel(preview_model) : previews::Preview::Ptr();
  
  if (preview_view)
  {
    preview_view->request_close().connect([this]() { request_close.emit(); });
  }
  
  preview_layout_->PushPreview(preview_view, direction);
}

void PreviewContainer::DisableNavButton(Navigation button)
{
  nav_disabled_ = button;
  nav_right_->SetEnabled(IsNavigationDisabled(Navigation::RIGHT) == false);
  nav_left_->SetEnabled(IsNavigationDisabled(Navigation::LEFT) == false);
  QueueDraw();
}

bool PreviewContainer::IsNavigationDisabled(Navigation button) const
{
  return (nav_disabled_ & button) != Navigation::NONE;  
}

std::string PreviewContainer::GetName() const
{
  return "PreviewContainer";
}

void PreviewContainer::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
    .add(GetAbsoluteGeometry())
    .add("navigate-left-enabled", !IsNavigationDisabled(Navigation::LEFT))
    .add("navigate-right-enabled", !IsNavigationDisabled(Navigation::RIGHT));
}

void PreviewContainer::SetupViews()
{
  previews::Style& style = previews::Style::Instance();

  nux::VLayout* layout = new nux::VLayout();
  SetLayout(layout);
  layout->AddLayout(new nux::SpaceLayout(0,0,style.GetPreviewTopPadding(),style.GetPreviewTopPadding()));

  layout_content_ = new nux::HLayout();
  layout_content_->SetSpaceBetweenChildren(6);
  layout->AddLayout(layout_content_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

  layout_content_->AddSpace(0, 1);
  nav_left_ = new PreviewNavigator(Orientation::LEFT, NUX_TRACKER_LOCATION);
  AddChild(nav_left_);
  nav_left_->SetMinimumWidth(style.GetNavigatorWidth());
  nav_left_->SetMaximumWidth(style.GetNavigatorWidth());
  nav_left_->activated.connect([this]() { navigate_left.emit(); });
  layout_content_->AddView(nav_left_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

  preview_layout_ = new PreviewContent(this);
  preview_layout_->SetMinMaxSize(style.GetPreviewWidth(), style.GetPreviewHeight());
  AddChild(preview_layout_);
  layout_content_->AddLayout(preview_layout_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

  nav_right_ = new PreviewNavigator(Orientation::RIGHT, NUX_TRACKER_LOCATION);
  AddChild(nav_right_);
  nav_right_->SetMinimumWidth(style.GetNavigatorWidth());
  nav_right_->SetMaximumWidth(style.GetNavigatorWidth());
  nav_right_->activated.connect([this]() { navigate_right.emit(); });
  layout_content_->AddView(nav_right_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  layout_content_->AddSpace(0, 1);

  layout->AddSpace(0, 1);

  preview_layout_->start_navigation.connect([this]()
  {
    // reset animation clock.
    if (navigation_count_ == 0)
      clock_gettime(CLOCK_MONOTONIC, &last_progress_time_);
  
    float navigation_progress_remaining = CLAMP((1.0 - preview_layout_->GetAnimationProgress()) + navigation_count_, 1.0f, 10.0f);
    navigation_count_++;

    navigation_progress_speed_ = navigation_progress_remaining / ANIM_DURATION_LONG;
    QueueAnimation();
  });

  preview_layout_->continue_navigation.connect([this]()
  {
    QueueAnimation(); 
  });

  preview_layout_->end_navigation.connect([this]()
  {
    navigation_count_ = 0;
    navigation_progress_speed_ = 0;
  });

  navigate_right.connect( [this]() { preview_layout_->StartPreviewWait(); } );
  navigate_left.connect( [this]() { preview_layout_->StartPreviewWait(); } );
}

void PreviewContainer::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
}

void PreviewContainer::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

  bool redirect_to_texture = RedirectRenderingToTexture();

    // This is necessary when doing redirected rendering. Clean the area below this view.
  if (redirect_to_texture)
  {
    // This is necessary when doing redirected rendering.
    // Clean the area below this view before drawing anything.
    gfx_engine.GetRenderStates().SetBlend(false);
    gfx_engine.QRP_Color(GetX(), GetY(), GetWidth(), GetHeight(), nux::Color(0.0f, 0.0f, 0.0f, 0.0f));
  }
  
    // rely on the compiz event loop to come back to us in a nice throttling
  if (AnimationInProgress())
  {
    if (!animation_timer_)
       animation_timer_.reset(new glib::Timeout(1000/60, sigc::mem_fun(this, &PreviewContainer::QueueAnimation)));
  }
  else if (preview_layout_ && preview_layout_->IsAnimating())
  {
    preview_layout_->UpdateAnimationProgress(1.0f, 1.0f);
  }

  // Paint using ProcessDraw2. ProcessDraw is overrided  by empty impl so we can control z order.
  if (preview_layout_)
  {
    preview_layout_->ProcessDraw2(gfx_engine, force_draw || redirect_to_texture);
  }
  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw || RedirectRenderingToTexture());

  gfx_engine.PopClippingRectangle();
}

bool PreviewContainer::AnimationInProgress()
{
   // short circuit to avoid unneeded calculations
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  if (preview_layout_ == nullptr)
    return false;

  // hover in animation
  if (navigation_progress_speed_ > 0)
    return true;

  return false;
}

static float easeInOutQuart(float t)
{
    t = CLAMP(t, 0.0, 1.0);
    t*=2.0f;
    if (t < 1.0) return 0.5f*pow(t, 4);
    else {
        t -= 2.0f;
        return -0.5f * (pow(t, 4)- 2);
    }
}

float PreviewContainer::GetSwipeAnimationProgress(struct timespec const& current) const
{
  DeltaTime time_delta = TimeUtil::TimeDelta(&current, &last_progress_time_);
  float progress = preview_layout_->GetAnimationProgress() + (navigation_progress_speed_ * time_delta);

  return progress;
}

bool PreviewContainer::QueueAnimation()
{
  animation_timer_.reset();
  
  timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);
  float progress = GetSwipeAnimationProgress(current);
  preview_layout_->UpdateAnimationProgress(progress, easeInOutQuart(progress)); // ease in/out.
  last_progress_time_ = current;

  QueueDraw();
  return false;
}

bool PreviewContainer::AcceptKeyNavFocus()
{
  return true;
}

bool PreviewContainer::InspectKeyEvent(unsigned int eventType, unsigned int keysym, const char* character)
{
  switch (keysym)
  {
    case NUX_VK_ESCAPE:
      return true;
    default:
      break;
  }

  return false;
}

void PreviewContainer::OnKeyDown(unsigned long event_type, unsigned long event_keysym,
                                    unsigned long event_state, const TCHAR* character,
                                    unsigned short key_repeat_count)
{
  if (event_type == nux::NUX_KEYDOWN)
  {
    switch (event_keysym)
    {
      case NUX_VK_ESCAPE:
        request_close.emit();
        break;

      default:
        return;
    }
  }
}

nux::Area* PreviewContainer::FindKeyFocusArea(unsigned int key_symbol,
                                      unsigned long x11_key_code,
                                      unsigned long special_keys_state)
{
  nux::Area* area = preview_layout_->FindKeyFocusArea(key_symbol, x11_key_code, special_keys_state);
  if (area)
    return area;

  return this;
}

nux::Area* PreviewContainer::KeyNavIteration(nux::KeyNavDirection direction)
{
  using namespace nux;
  nux::Area* area = preview_layout_->KeyNavIteration(direction);
  if (area)
    return area;

  switch(direction)
  {
    case KEY_NAV_LEFT:
      if (!IsNavigationDisabled(Navigation::LEFT))
        navigate_left.emit();
      break;

    case KEY_NAV_RIGHT:
      if (!IsNavigationDisabled(Navigation::RIGHT))
        navigate_right.emit();
      break;

    default:
      break;
  }

  return this;
}

void PreviewContainer::OnMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  int button = nux::GetEventButton(button_flags);

  if (button == nux::MOUSE_BUTTON1 || button == nux::MOUSE_BUTTON2 || button == nux::MOUSE_BUTTON3)
  {
    request_close.emit();
  }
}

nux::Geometry PreviewContainer::GetLayoutGeometry() const
{
  return layout_content_->GetAbsoluteGeometry();  
}

} // namespace previews
} // namespace dash
} // namespace unity

