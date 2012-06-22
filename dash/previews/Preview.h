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

#ifndef PREVIEW_H
#define PREVIEW_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <UnityCore/Preview.h>
#include "unity-shared/Introspectable.h"

namespace unity
{
namespace dash
{
namespace previews
{
typedef enum 
{
  LEFT,
  RIGHT,
  BOTH
} NavButton;

class Preview : public nux::View, public debug::Introspectable
{
public:
  typedef nux::ObjectPtr<Preview> Ptr;
  NUX_DECLARE_OBJECT_TYPE(Preview, nux::View);

  Preview(dash::Preview::Ptr preview_model_);
  virtual ~Preview();

  // calling this should disable the nav buttons to the left or the right of the preview
  virtual void DisableNavButton(NavButton button);
 
  // For the nav buttons to the left/right of the previews, call when they are activated
  sigc::signal<void> navigate_left;
  sigc::signal<void> navigate_right;

  // for introspection
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

protected:
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
};

}
}
}

#endif //PREVIEW_H
