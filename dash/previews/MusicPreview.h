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

#ifndef MUSICPREVIEW_H
#define MUSICPREVIEW_H

#include "Preview.h"
#include "unity-shared/IconTexture.h"

namespace unity
{
namespace dash
{
namespace previews
{
class Tracks;

class MusicPreview : public Preview
{
public:
  typedef nux::ObjectPtr<MusicPreview> Ptr;
  NUX_DECLARE_OBJECT_TYPE(MusicPreview, Preview);

  MusicPreview(dash::Preview::Ptr preview_model);
  ~MusicPreview();

  // From debug::Introspectable
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

protected:
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void PreLayoutManagement();

  virtual void SetupViews();

  virtual void OnNavigateOut();

protected:
  nux::ObjectPtr<Tracks> tracks_;
  nux::ObjectPtr<StaticCairoText> warning_msg_;
  nux::ObjectPtr<IconTexture> warning_texture_;
  std::string no_credentials_message_;

 };

}
}
}

#endif // MUSICPREVIEW_H
