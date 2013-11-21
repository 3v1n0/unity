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

#ifndef MOVIEPREVIEW_H
#define MOVIEPREVIEW_H

#include "Preview.h"

namespace unity
{
namespace dash
{
namespace previews
{
class PreviewRatingsWidget;

class MoviePreview : public Preview
{
public:
  typedef nux::ObjectPtr<MoviePreview> Ptr;
  NUX_DECLARE_OBJECT_TYPE(MoviePreview, Preview);

  MoviePreview(dash::Preview::Ptr preview_model);
  ~MoviePreview();

  // From debug::Introspectable
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

protected:
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void PreLayoutManagement();

  virtual void OnNavigateOut();
  virtual void OnNavigateInComplete();

  virtual void SetupViews();
  
protected:
  nux::ObjectPtr<PreviewRatingsWidget> rating_;
};

}
}
}

#endif // MOVIEPREVIEW_H
