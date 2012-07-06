// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2011 Canonical Ltd.
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
#include "PreviewNavigator.h"
#include "PreviewFactory.h"

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
nux::logging::Logger logger("unity.dash.previews.previewcontainer");

const int ANIM_DURATION_SHORT_SHORT = 100;
const int ANIM_DURATION = 200;
const int ANIM_DURATION_LONG = 350;

const std::string ANIMATION_IDLE = "animation-idle";
}

class PreviewContent : public nux::Layout
{
public:
  PreviewContent()
  : progress_(0.0)
  , animating_(false) {}

  void PushPreview(previews::Preview::Ptr preview, NavButton direction)
  {
    AddView(preview.GetPointer());

    preview->SetVisible(false);
    PreviewSwipe swipe;
    swipe.direction = direction;
    swipe.preview = preview;
    push_preview_.push(swipe);

    if (!animating_)
    {
      UpdateAnimationProgress(0.0);
    }
  }

  bool IsAnimating() { return animating_; }

  int SwipeQueueSize() const { return push_preview_.size(); }

  void UpdateAnimationProgress(float progress)
  {
    progress_ = progress;

    if (!animating_)
    {
      if (push_preview_.size())
      {
        animating_= true;
        swipe_ = push_preview_.front();
        push_preview_.pop();

        start_navigation.emit();
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
        if (swipe_.direction == RIGHT)
          swipeOut.OffsetPosition(-(progress_ * geometry.GetWidth()), 0);
        else if (swipe_.direction == LEFT)
          swipeOut.OffsetPosition(progress_*geometry.GetWidth(), 0);
        current_preview_->SetGeometry(swipeOut);
      }
 
       // swipe in
      if (swipe_.preview)
      {
        swipe_.preview->SetVisible(true);

        nux::Geometry swipeIn = geometry;
        if (swipe_.direction == RIGHT)
          swipeIn.OffsetPosition(float(geometry.GetWidth()) - (progress_ * geometry.GetWidth()), 0);
        else if (swipe_.direction == LEFT)
          swipeIn.OffsetPosition(-((1.0-progress_)*geometry.GetWidth()), 0);
        swipe_.preview->SetGeometry(swipeIn);
      }
    }

    if (progress >= 1.0)
    {
      animating_ = false;
      if (swipe_.preview)
      {
        if (current_preview_)
          RemoveChildObject(current_preview_.GetPointer());
        current_preview_ = swipe_.preview;
      } 

      // another swipe?
      if (push_preview_.size())
      {
        progress_ = 0;
        start_navigation.emit();
      }

      // set the geometry to the whole layout.
      if (current_preview_ && current_preview_->GetGeometry() != geometry)
      {
        current_preview_->SetGeometry(geometry);
      }
    }
  }

  // void ProcessDraw(nux::GraphicsEngine& gfx_engine, bool force_draw)
  // {
  //   if (swipe_.preview && swipe_.preview->IsVisible()) { swipe_.preview->ProcessDraw(gfx_engine, force_draw); }
  //   if (current_preview_) {  current_preview_->ProcessDraw(gfx_engine, force_draw); }
  // }

  sigc::signal<void> start_navigation;

protected:
private:
  struct PreviewSwipe
  {
    NavButton direction;
    previews::Preview::Ptr preview;

    void reset() { preview.Release(); }
  };
  std::queue<PreviewSwipe> push_preview_;

  previews::Preview::Ptr current_preview_;
  PreviewSwipe swipe_;

  float progress_;
  bool animating_;
};

NUX_IMPLEMENT_OBJECT_TYPE(PreviewContainer);

PreviewContainer::PreviewContainer(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
{
  SetupViews();

}

PreviewContainer::~PreviewContainer()
{
}

void PreviewContainer::preview(glib::Variant const& preview, NavButton direction)
{
  PreviewFactoryOperator previewOperator(PreviewFactory::Instance().Item(preview));

  dash::Preview::Ptr model = previewOperator.CreateModel();
  current_preview_ = previewOperator.CreateView(model);

  content_layout_->PushPreview(current_preview_, direction);
}

void PreviewContainer::DisableNavButton(NavButton button)
{
}

void PreviewContainer::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
}

void PreviewContainer::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  // rely on the compiz event loop to come back to us in a nice throttling
  if (AnimationInProgress())
  {
    auto idle = std::make_shared<glib::Idle>(glib::Source::Priority::DEFAULT);
    sources_.Add(idle, ANIMATION_IDLE);
    idle->Run([&]() {
      EnsureAnimation();
      return false;
    });

    timespec current;
    clock_gettime(CLOCK_MONOTONIC, &current);

    content_layout_->UpdateAnimationProgress(GetSwipeAnimationProgress(current));
  }
  else if (content_layout_->IsAnimating())
  {
    content_layout_->UpdateAnimationProgress(1.0f);
  }

  nux::Geometry base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);

  gfx_engine.PopClippingRectangle();
}

float PreviewContainer::GetSwipeAnimationProgress(struct timespec const& current) const
{
  return CLAMP((float)(unity::TimeUtil::TimeDelta(&current, &_times[TIME_PREVIEW_NAVIGATE])) / (float) ANIM_DURATION_LONG, 0.0f, 1.0f);
}


void PreviewContainer::EnsureAnimation()
{
  QueueDraw();
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
  previews::Style& stlye = previews::Style::Instance();

  layout_ = new nux::HLayout();
  SetLayout(layout_);

  nav_left_ = new PreviewNavigator(Orientation::LEFT, NUX_TRACKER_LOCATION);
  nav_left_->SetMinimumWidth(stlye.NavigatorMinimumWidth());
  nav_left_->SetMaximumWidth(stlye.NavigatorMaximumWidth());
  nav_left_->activated.connect([&]() { navigate_left.emit(); });
  layout_->AddView(nav_left_, 0);

  content_layout_ = new PreviewContent();
  layout_->AddLayout(content_layout_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  nav_right_ = new PreviewNavigator(Orientation::RIGHT, NUX_TRACKER_LOCATION);
  nav_right_->SetMinimumWidth(stlye.NavigatorMinimumWidth());
  nav_right_->SetMaximumWidth(stlye.NavigatorMaximumWidth());
  nav_right_->activated.connect([&]() { navigate_right.emit(); });
  layout_->AddView(nav_right_, 0);

  content_layout_->start_navigation.connect([&]()
  {
    TimeUtil::SetTimeStruct(&_times[TIME_PREVIEW_NAVIGATE], &_times[TIME_PREVIEW_NAVIGATE], ANIM_DURATION_LONG);
    EnsureAnimation();
  });
}

bool PreviewContainer::AnimationInProgress()
{
   // short circuit to avoid unneeded calculations
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  // hover in animation
  if (unity::TimeUtil::TimeDelta(&current, &_times[TIME_PREVIEW_NAVIGATE]) < ANIM_DURATION_LONG)
    return true;

  return false;
}



}
}
}
