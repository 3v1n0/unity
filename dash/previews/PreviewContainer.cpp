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

class PreviewContent : public nux::Layout
{
public:
  PreviewContent(const PreviewContainer*const parent)
  : progress_(0.0)
  , animating_(false)
  , parent_(parent) {}

  void PushPreview(previews::Preview::Ptr preview, Navigation direction)
  {
    if (preview)
    {
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
      if (push_preview_.size())
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
          swipeOut.OffsetPosition(-(curve_progress * (geometry.GetWidth() + parent_->nav_left_->GetGeometry().GetWidth())), 0);
        else if (swipe_.direction == Navigation::LEFT)
          swipeOut.OffsetPosition(curve_progress* (geometry.GetWidth() + parent_->nav_right_->GetGeometry().GetWidth()), 0);
        current_preview_->SetGeometry(swipeOut);
      }
 
       // swipe in
      if (swipe_.preview)
      {
        swipe_.preview->SetVisible(true);

        nux::Geometry swipeIn = geometry;
        if (swipe_.direction == Navigation::RIGHT)
          swipeIn.OffsetPosition(float(geometry.GetWidth() + parent_->nav_right_->GetGeometry().GetWidth()) - (curve_progress * (geometry.GetWidth() + parent_->nav_right_->GetGeometry().GetWidth())), 0);
        else if (swipe_.direction == Navigation::LEFT)
          swipeIn.OffsetPosition(-((1.0-curve_progress)*(geometry.GetWidth() + parent_->nav_left_->GetGeometry().GetWidth())), 0);
        swipe_.preview->SetGeometry(swipeIn);
      }
    }

    if (progress >= 1.0)
    {
      animating_ = false;
      if (current_preview_)
      {
        RemoveChildObject(current_preview_.GetPointer());
        current_preview_.Release();
      }
      if (swipe_.preview)
      {
        current_preview_ = swipe_.preview;
        swipe_.preview.Release();
        if (current_preview_)
          current_preview_->OnNavigateInComplete();
      } 

      // another swipe?
      if (push_preview_.size())
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
    }
  }

  // Dont draw in process draw. this is so we can control the z order.
  void ProcessDraw(nux::GraphicsEngine& gfx_engine, bool force_draw)
  {
  }

  void ProcessDraw2(nux::GraphicsEngine& gfx_engine, bool force_draw)
  {
    if (swipe_.preview && swipe_.preview->IsVisible()) { swipe_.preview->ProcessDraw(gfx_engine, force_draw); }
    if (current_preview_ && current_preview_->IsVisible()) { current_preview_->ProcessDraw(gfx_engine, force_draw); }

    _queued_draw = false;
  }

  sigc::signal<void> start_navigation;
  sigc::signal<void> continue_navigation;
  sigc::signal<void> end_navigation;

protected:
private:
  struct PreviewSwipe
  {
    Navigation direction;
    previews::Preview::Ptr preview;

    void reset() { preview.Release(); }
  };
  std::queue<PreviewSwipe> push_preview_;

  previews::Preview::Ptr current_preview_;
  PreviewSwipe swipe_;

  float progress_;
  bool animating_;
  const PreviewContainer*const parent_;
};

NUX_IMPLEMENT_OBJECT_TYPE(PreviewContainer);

PreviewContainer::PreviewContainer(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , content_layout_(nullptr)
  , nav_disabled_(Navigation::NONE)
  , navigation_progress_speed_(0.0)
  , navigation_count_(0)
{
  SetupViews();
  last_progress_time_.tv_sec = 0;
  last_progress_time_.tv_nsec = 0;

  key_down.connect(sigc::mem_fun(this, &PreviewContainer::OnKeyDown));
}

PreviewContainer::~PreviewContainer()
{
}

void PreviewContainer::Preview(dash::Preview::Ptr preview_model, Navigation direction)
{
  previews::Preview::Ptr preview_view = previews::Preview::PreviewForModel(preview_model);
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
}

void PreviewContainer::SetupViews()
{
  previews::Style& style = previews::Style::Instance();

  layout_ = new nux::HLayout();
  SetLayout(layout_);

  nav_left_ = new PreviewNavigator(Orientation::LEFT, NUX_TRACKER_LOCATION);
  nav_left_->SetMinimumWidth(style.GetNavigatorWidth());
  nav_left_->SetMaximumWidth(style.GetNavigatorWidth());
  nav_left_->activated.connect([&]() { navigate_left.emit(); });
  layout_->AddView(nav_left_, 0);

  content_layout_ = new PreviewContent(this);
  layout_->AddLayout(content_layout_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  nav_right_ = new PreviewNavigator(Orientation::RIGHT, NUX_TRACKER_LOCATION);
  nav_right_->SetMinimumWidth(style.GetNavigatorWidth());
  nav_right_->SetMaximumWidth(style.GetNavigatorWidth());
  nav_right_->activated.connect([&]() { navigate_right.emit(); });
  layout_->AddView(nav_right_, 0);

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
}

void PreviewContainer::ProcessDraw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  bool bNextQueue = AnimationInProgress();
  // rely on the compiz event loop to come back to us in a nice throttling
  if (!bNextQueue && content_layout_ && content_layout_->IsAnimating())
  {
    content_layout_->UpdateAnimationProgress(1.0f, 1.0f);
  }

  View::ProcessDraw(gfx_engine, force_draw);

  if (bNextQueue)
  {
    QueueAnimation();
  }
}

void PreviewContainer::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
}

void PreviewContainer::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

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

// static float easeInOutCubic(float t)
// {
//     t = CLAMP(t, 0.0, 1.0);
//     t*=2.0f;
//     if(t < 1.0f) {
//         return 0.5f*pow(t, 3);
//     } else {
//         t -= 2.0f;
//         return 0.5f*(pow(t, 3) + 2);
//     }
// }

float PreviewContainer::GetSwipeAnimationProgress(struct timespec const& current) const
{
  int time_delta = TimeUtil::TimeDelta(&current, &last_progress_time_);
  float progress = content_layout_->GetAnimationProgress() + (navigation_progress_speed_ * time_delta);

  return progress;
}

void PreviewContainer::QueueAnimation()
{
//   if (!idle_)
//   {
//     idle_.reset(new glib::Idle(sigc::mem_fun(this, &PreviewContainer::Iteration), glib::Source::Priority::DEFAULT));
//   }
// }

// bool PreviewContainer::Iteration()
// {
  // idle_.reset();

  timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);
  float progress = GetSwipeAnimationProgress(current);
  content_layout_->UpdateAnimationProgress(progress, easeInOutQuart(progress)); // ease in/out.
  last_progress_time_ = current;

  QueueDraw();
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

nux::Area* PreviewContainer::KeyNavIteration(nux::KeyNavDirection direction)
{
  return this;
}

} // namespace previews
} // namespace dash
} // namespace unity