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

#ifndef PREVIEWCONTAINER_H
#define PREVIEWCONTAINER_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <Nux/VLayout.h>
#include <NuxCore/Animation.h>
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

enum class Navigation : unsigned int
{
  NONE = 0,
  LEFT = (1<<0),
  RIGHT = (1<<1),
  BOTH = LEFT|RIGHT
};

class PreviewContainer : public nux::View, public debug::Introspectable
{
public:
  typedef nux::ObjectPtr<PreviewContainer> Ptr;
  NUX_DECLARE_OBJECT_TYPE(PreviewContainer, nux::View);

  PreviewContainer(NUX_FILE_LINE_PROTO);

  void Preview(dash::Preview::Ptr preview_model, Navigation direction);

  // calling this should disable the nav buttons to the left or the right of the preview
  void DisableNavButton(Navigation button);
  bool IsNavigationDisabled(Navigation button) const;

  // From debug::Introspectable
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);


  // For the nav buttons to the left/right of the previews, call when they are activated
  sigc::signal<void> navigate_left;
  sigc::signal<void> navigate_right;
  sigc::signal<void> request_close;

  nux::Property<double> scale;

  bool AcceptKeyNavFocus();

  nux::Area* KeyNavIteration(nux::KeyNavDirection direction);
  nux::Area* FindKeyFocusArea(unsigned int key_symbol,
                                      unsigned long x11_key_code,
                                      unsigned long special_keys_state);

  nux::Geometry GetLayoutGeometry() const;

  void OnMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags);

protected:
  void Draw(nux::GraphicsEngine& gfx_engine, bool force_draw);
  void DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw);

  bool InspectKeyEvent(unsigned int eventType, unsigned int keysym, const char* character);
  void OnKeyDown(unsigned long event_type, unsigned long event_keysym, unsigned long event_state, const TCHAR* character, unsigned short key_repeat_count);

private:
  void SetupViews();

  void QueueAnimation(double progress);
  double GetSwipeAnimationProgress(struct timespec const& current) const;

private:
  void UpdateScale(double scale);

  // View related
  nux::HLayout* layout_content_;
  PreviewNavigator* nav_left_;
  PreviewNavigator* nav_right_;
  PreviewContent* preview_layout_;
  Navigation nav_disabled_;

  // Animation
  nux::animation::AnimateValue<double> animation_;

  friend class PreviewContent;
};

} // namespace previews
} // namespace dash
} // namespace unity

#endif //PREVIEWCONTAINER_H
