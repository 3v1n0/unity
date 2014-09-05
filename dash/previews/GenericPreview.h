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

#ifndef GENERICPREVIEW_H
#define GENERICPREVIEW_H

#include <UnityCore/GenericPreview.h>
#include "Preview.h"
#include "unity-shared/OverlayScrollView.h"

namespace unity
{
namespace dash
{
namespace previews
{

class GenericPreview : public Preview
{
public:
  typedef nux::ObjectPtr<GenericPreview> Ptr;
  NUX_DECLARE_OBJECT_TYPE(GenericPreview, Preview);

  GenericPreview(dash::Preview::Ptr preview_model);
  ~GenericPreview();

  // From debug::Introspectable
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

protected:
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void PreLayoutManagement();

  virtual void SetupViews();
  virtual void UpdateScale(double scale);

  nux::HLayout* image_data_layout_;
  nux::VLayout* preview_info_layout_;
  ScrollView* preview_info_scroll_;
  nux::VLayout* preview_data_layout_;
  nux::Layout* actions_layout_;
};

}
}
}

#endif // GENERICPREVIEW_H
