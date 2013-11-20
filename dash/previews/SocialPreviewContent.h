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
 *              Ken VanDine <ken.vandine@canonical.com>
 *
 */

#ifndef SOCIAL_PREVIEW_CONTENT_H
#define SOCIAL_PREVIEW_CONTENT_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <Nux/CairoWrapper.h>
#include <NuxCore/ObjectPtr.h>
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/Introspectable.h"
#include "PreviewContainer.h"

namespace unity
{
namespace dash
{
namespace previews
{

class SocialPreviewContent : public nux::View, public unity::debug::Introspectable
{
public:
  typedef nux::ObjectPtr<SocialPreviewContent> Ptr;
  NUX_DECLARE_OBJECT_TYPE(SocialPreviewContent, nux::View);

  SocialPreviewContent(std::string const& text, NUX_FILE_LINE_PROTO);
  virtual ~SocialPreviewContent();

  void SetText(std::string const& text);

  sigc::signal<void> request_close() const { return preview_container_.request_close; }

protected:
  virtual void Draw(nux::GraphicsEngine& gfx_engine, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw);
  virtual void PreLayoutManagement();

  virtual bool AcceptKeyNavFocus() { return false; }

  void SetupViews();
  void UpdateBaloonTexture();
  void RedrawBubble(nux::Geometry const& geom, cairo_t* cr, nux::ButtonVisualState faked_state);
  void DrawBubble(cairo_t* cr,
                   double line_width,
                   double   radius,
                   double   x,
                   double   y,
                   double   width,
                   double   height,
                   double   tailPosition,
                   double   tailWidth);

  virtual std::string GetName() const;
  virtual void AddProperties(debug::IntrospectionData&);

private:
  nux::ObjectPtr<StaticCairoText> text_;

  typedef std::unique_ptr<nux::CairoWrapper> NuxCairoPtr;
  NuxCairoPtr cr_bubble_;

  PreviewContainer preview_container_;
};

}
}
}

#endif // SOCIAL_PREVIEW_CONTENT_H
