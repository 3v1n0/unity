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

namespace nux
{
class AbstractButton;
}

namespace unity
{
namespace dash
{
namespace previews
{

class Preview : public nux::View, public debug::Introspectable
{
public:
  typedef nux::ObjectPtr<Preview> Ptr;
  NUX_DECLARE_OBJECT_TYPE(Preview, nux::View);

  Preview(dash::Preview::Ptr preview_model);
  virtual ~Preview();
 
  // From debug::Introspectable
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

protected:
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {}
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {}
  
  virtual void OnActionActivated(nux::AbstractButton* button, std::string const& id);

  virtual void OnNavigateIn() {}
  virtual void OnNavigateInComplete() {}
  virtual void OnNavigateOut() {}
  
protected:
  dash::Preview::Ptr preview_model_;

  friend class PreviewContent;
};

}
}
}

#endif // PREVIEW_H
