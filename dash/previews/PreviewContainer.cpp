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

#include "unity-shared/IntrospectableWrappers.h"
#include "unity-shared/TimeUtil.h"
#include "unity-shared/PreviewStyle.h"
#include "PreviewNavigator.h"
#include <boost/math/constants/constants.hpp>
#include "config.h"

namespace unity
{
namespace dash
{
namespace previews
{

Navigation operator&(const Navigation lhs, const Navigation rhs)
{
  return Navigation(int(lhs) & int(rhs));
}

namespace
{
nux::logging::Logger logger("unity.dash.previews.previewcontainer");



const int ANIM_DURATION_SHORT_SHORT = 100;
const int ANIM_DURATION = 200;
const int ANIM_DURATION_LONG = 500;

const std::string ANIMATION_IDLE = "animation-idle";
}

class PreviewContent : public nux::Layout, public debug::Introspectable
{
public:
  PreviewContent(PreviewContainer*const parent)
  : parent_(parent)
  , progress_(0.0)
  , animating_(false)
  , waiting_preview_(false)
  , rotation_(0.0)
  , preview_initiate_count_(0)
  , nav_complete_(0)
  , relative_nav_index_(0)
  {
    Style& style = previews::Style::Instance();

    spin_= style.GetSearchSpinIcon(256);
  }

  // From debug::Introspectable
  std::string GetName() const
  {
    return "PreviewContent";
  }

  void AddProperties(GVariantBuilder* builder)
  {
    variant::BuilderWrapper(builder)
      .add("animating", animating_)
      .add("animation_progress", progress_)
      .add("waiting_preview", waiting_preview_)
      .add("preview-initiate-count", preview_initiate_count_)
      .add("navigation-complete-count", nav_complete_)
      .add("relative-nav-index", relative_nav_index_);
  }

  void PushPreview(previews::Preview::Ptr preview, Navigation direction)
  {
    preview_initiate_count_++;
    StopPreviewWait();

    if (preview)
    {
      AddChild(preview.GetPointer());
      AddView(preview.GetPointer());
      preview->SetVisible(false);
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

      // set the geometry to the whole layout.
      if (current_preview_ && current_preview_->GetGeometry() != geometry)
      {
        current_preview_->SetGeometry(geometry);
      }
      nav_complete_++;
    }
  }

  void StartPreviewWait()
  {
    preview_wait_timer_.reset(new glib::Timeout(300, [&] () {

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

      gfx_engine.PushModelViewMatrix(nux::Matrix4::TRANSLATE(-spin_geo.x - (spin_geo.width + spin_offset_w) / 2.0f,
                                                             -spin_geo.y - (spin_geo.height + spin_offset_h) / 2.0f, 0));
      gfx_engine.PushModelViewMatrix(rotate_matrix_);
      gfx_engine.PushModelViewMatrix(nux::Matrix4::TRANSLATE(spin_geo.x + (spin_geo.width + spin_offset_w) / 2.0f,
                                                             spin_geo.y + (spin_geo.height + spin_offset_h) / 2.0f, 0));

      gfx_engine.QRP_1Tex(spin_geo.x,
                          spin_geo.y,
                          spin_geo.width,
                          spin_geo.height,
                          spin_->GetDeviceTexture(),
                          texxform,
                          nux::color::White);

      gfx_engine.PopModelViewMatrix();
      gfx_engine.PopModelViewMatrix();
      gfx_engine.PopModelViewMatrix();

      gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);

      if (!frame_timeout_)
      {
        frame_timeout_.reset(new glib::Timeout(22, sigc::mem_fun(this, &PreviewContent::OnFrameTimeout)));
      }
    }

    _queued_draw = false;
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
  , content_layout_(nullptr)
  , nav_disabled_(Navigation::NONE)
  , navigation_progress_speed_(0.0)
  , navigation_count_(0)
{
  SetAcceptKeyNavFocusOnMouseDown(false);
  SetAcceptKeyNavFocusOnMouseEnter(true);

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
  previews::Preview::Ptr preview_view = previews::Preview::PreviewForModel(preview_model);
  
  if (preview_view)
  {
    preview_view->request_close.connect([this]() { request_close.emit(); });
  }
  
  content_layout_->PushPreview(preview_view, direction);
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

void PreviewContainer::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder)
    .add(GetAbsoluteGeometry())
    .add("navigate-left-enabled", !IsNavigationDisabled(Navigation::LEFT))
    .add("navigate-right-enabled", !IsNavigationDisabled(Navigation::RIGHT));
}

void PreviewContainer::SetupViews()
{
  previews::Style& style = previews::Style::Instance();

  layout_ = new nux::HLayout();
  SetLayout(layout_);
  layout_->AddSpace(0, 1);

  nav_left_ = new PreviewNavigator(Orientation::LEFT, NUX_TRACKER_LOCATION);
  AddChild(nav_left_);
  nav_left_->SetMinimumWidth(style.GetNavigatorWidth());
  nav_left_->SetMaximumWidth(style.GetNavigatorWidth());
  nav_left_->activated.connect([&]() { navigate_left.emit(); });
  layout_->AddView(nav_left_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  content_layout_ = new PreviewContent(this);
  content_layout_->SetMinMaxSize(style.GetPreviewWidth(), style.GetPreviewHeight());
  AddChild(content_layout_);
  layout_->AddLayout(content_layout_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  nav_right_ = new PreviewNavigator(Orientation::RIGHT, NUX_TRACKER_LOCATION);
  AddChild(nav_right_);
  nav_right_->SetMinimumWidth(style.GetNavigatorWidth());
  nav_right_->SetMaximumWidth(style.GetNavigatorWidth());
  nav_right_->activated.connect([&]() { navigate_right.emit(); });
  layout_->AddView(nav_right_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  layout_->AddSpace(0, 1);

  content_layout_->start_navigation.connect([&]()
  {
    // reset animation clock.
    if (navigation_count_ == 0)
      clock_gettime(CLOCK_MONOTONIC, &last_progress_time_);
  
    float navigation_progress_remaining = CLAMP((1.0 - content_layout_->GetAnimationProgress()) + navigation_count_, 1.0f, 10.0f);
    navigation_count_++;

    navigation_progress_speed_ = navigation_progress_remaining / ANIM_DURATION_LONG;
    QueueAnimation();
  });

  content_layout_->continue_navigation.connect([&]()
  {
    QueueAnimation(); 
  });

  content_layout_->end_navigation.connect([&]()
  {
    navigation_count_ = 0;
    navigation_progress_speed_ = 0;
  });

  navigate_right.connect( [&]() { content_layout_->StartPreviewWait(); } );
  navigate_left.connect( [&]() { content_layout_->StartPreviewWait(); } );
}

void PreviewContainer::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();

  gfx_engine.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(gfx_engine, geo);
  gfx_engine.PopClippingRectangle();
}

void PreviewContainer::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

    // rely on the compiz event loop to come back to us in a nice throttling
  if (AnimationInProgress())
  {
    if (!animation_timer_)
       animation_timer_.reset(new glib::Timeout(1000/60, sigc::mem_fun(this, &PreviewContainer::QueueAnimation)));
  }
  else if (content_layout_ && content_layout_->IsAnimating())
  {
    content_layout_->UpdateAnimationProgress(1.0f, 1.0f);
  }

  // Paint using ProcessDraw2. ProcessDraw is overrided  by empty impl so we can control z order.
  if (content_layout_)
  {
    content_layout_->ProcessDraw2(gfx_engine, force_draw);
  }
  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);

  gfx_engine.PopClippingRectangle();
}

bool PreviewContainer::AnimationInProgress()
{
   // short circuit to avoid unneeded calculations
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  if (content_layout_ == nullptr)
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
  int time_delta = TimeUtil::TimeDelta(&current, &last_progress_time_);
  float progress = content_layout_->GetAnimationProgress() + (navigation_progress_speed_ * time_delta);

  return progress;
}

bool PreviewContainer::QueueAnimation()
{
  animation_timer_.reset();
  
  timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);
  float progress = GetSwipeAnimationProgress(current);
  content_layout_->UpdateAnimationProgress(progress, easeInOutQuart(progress)); // ease in/out.
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
  nux::KeyNavDirection direction = nux::KEY_NAV_NONE;
  switch (keysym)
  {
    case NUX_VK_UP:
      direction = nux::KeyNavDirection::KEY_NAV_UP;
      break;
    case NUX_VK_DOWN:
      direction = nux::KeyNavDirection::KEY_NAV_DOWN;
      break;
    case NUX_VK_LEFT:
      direction = nux::KeyNavDirection::KEY_NAV_LEFT;
      break;
    case NUX_VK_RIGHT:
      direction = nux::KeyNavDirection::KEY_NAV_RIGHT;
      break;
    case NUX_VK_LEFT_TAB:
      direction = nux::KeyNavDirection::KEY_NAV_TAB_PREVIOUS;
      break;
    case NUX_VK_TAB:
      direction = nux::KeyNavDirection::KEY_NAV_TAB_NEXT;
      break;
    case NUX_VK_ENTER:
    case NUX_KP_ENTER:
      direction = nux::KeyNavDirection::KEY_NAV_ENTER;
      break;
    case NUX_VK_ESCAPE:
      return true;
    default:
      direction = nux::KeyNavDirection::KEY_NAV_NONE;
      break;
  }

  if (direction == nux::KeyNavDirection::KEY_NAV_NONE
      || direction == nux::KeyNavDirection::KEY_NAV_UP
      || direction == nux::KeyNavDirection::KEY_NAV_DOWN
      || direction == nux::KeyNavDirection::KEY_NAV_TAB_NEXT
      || direction == nux::KeyNavDirection::KEY_NAV_TAB_PREVIOUS
      || direction == nux::KeyNavDirection::KEY_NAV_ENTER)
  {
    // we don't handle these cases
    return false;
  }

   // check for edge cases where we want the keynav to bubble up
  if (direction == nux::KEY_NAV_LEFT && IsNavigationDisabled(Navigation::LEFT))
    return false; // pressed left on the first item, no diiice
  else if (direction == nux::KEY_NAV_RIGHT && IsNavigationDisabled(Navigation::RIGHT))
    return false; // pressed right on the last item, nope. nothing for you

  return true;
}

void PreviewContainer::OnKeyDown(unsigned long event_type, unsigned long event_keysym,
                                    unsigned long event_state, const TCHAR* character,
                                    unsigned short key_repeat_count)
{
  if (event_type == nux::NUX_KEYDOWN)
  {
    switch (event_keysym)
    {
      case NUX_VK_LEFT:
        navigate_left.emit();
        break;

      case NUX_VK_RIGHT:
        navigate_right.emit();
        break;

      case NUX_VK_ESCAPE:
        request_close.emit();
        break;

      default:
        return;
    }
  }
}

void PreviewContainer::OnMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  if (nux::GetEventButton(button_flags) == nux::MOUSE_BUTTON1 || nux::GetEventButton(button_flags) == nux::MOUSE_BUTTON3)
  {
    request_close.emit();
  }
}

nux::Area* PreviewContainer::KeyNavIteration(nux::KeyNavDirection direction)
{
  return this;
}

} // namespace previews
} // namespace dash
} // namespace unity

