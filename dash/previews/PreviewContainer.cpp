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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
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
const int ANIM_DURATION_LONG = 10000;

const std::string ANIMATION_IDLE = "animation-idle";
}

class PreviewContent : private nux::HLayout
{
public:
  PreviewContent() {}

  void SetLeftPreview();
  void SetRightPreview();

private:
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

void PreviewContainer::preview(glib::Variant const& preview)
{
  PreviewFactoryOperator previewOperator(PreviewFactory::Instance().Item(preview));

  dash::Preview::Ptr model = previewOperator.CreateModel();

  content_layout_->RemoveChildObject(current_preview_.GetPointer());
  current_preview_ = previewOperator.CreateView(model);
  if (current_preview_)
  {
    content_layout_->AddView(current_preview_.GetPointer(), 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  }
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
  }


  nux::Geometry base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);

  gfx_engine.PopClippingRectangle();
}

void PreviewContainer::EnsureAnimation()
{
  LOG_WARN(logger) << "draw!";
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

  content_layout_ = new nux::HLayout();
  layout_->AddLayout(content_layout_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  nav_right_ = new PreviewNavigator(Orientation::RIGHT, NUX_TRACKER_LOCATION);
  nav_right_->SetMinimumWidth(stlye.NavigatorMinimumWidth());
  nav_right_->SetMaximumWidth(stlye.NavigatorMaximumWidth());
  nav_right_->activated.connect([&]() { navigate_right.emit(); });
  layout_->AddView(nav_right_, 0);
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
