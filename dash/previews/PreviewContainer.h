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

#ifndef PREVIEWCONTAINER_H
#define PREVIEWCONTAINER_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <UnityCore/Preview.h>
#include "Preview.h"
#include "unity-shared/Introspectable.h"

#include <UnityCore/GLibSource.h>

namespace unity
{
namespace dash
{
namespace previews
{

class PreviewNavigator;
class PreviewContent;

typedef enum 
{
  LEFT = (1<<0),
  RIGHT = (1<<1),
  BOTH = LEFT|RIGHT
} NavButton;

class PreviewContainer : public nux::View, public debug::Introspectable
{
public:
  typedef nux::ObjectPtr<PreviewContainer> Ptr;
  NUX_DECLARE_OBJECT_TYPE(PreviewContainer, nux::View);

  PreviewContainer(NUX_FILE_LINE_PROTO);
  virtual ~PreviewContainer();

  void preview(glib::Variant const& preview, NavButton direction);

  // calling this should disable the nav buttons to the left or the right of the preview
  virtual void DisableNavButton(NavButton button);

  // From debug::Introspectable
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  // For the nav buttons to the left/right of the previews, call when they are activated
  sigc::signal<void> navigate_left;
  sigc::signal<void> navigate_right;

protected:
  virtual void Draw(nux::GraphicsEngine& gfx_engine, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw);

private:
  void SetupViews();

  bool AnimationInProgress();
  void EnsureAnimation();
  float GetSwipeAnimationProgress(struct timespec const& current) const;

private:
  // View related
  nux::HLayout* layout_;
  PreviewNavigator* nav_left_;
  PreviewNavigator* nav_right_;
  PreviewContent* content_layout_;

  // Animation
  struct timespec  last_progress_time_;
  float navigation_progress_speed_;
  int navigation_count_;
  
  glib::SourceManager sources_;
  friend class PreviewContent;
};

} // napespace prviews
} // namespace dash
} // namespace unity

#endif //PREVIEWCONTAINER_H
