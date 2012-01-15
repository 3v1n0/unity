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

#include "PreviewBasicButton.h"

#include "config.h"
#include <Nux/Nux.h>

#include "cairo.h"
#include "DashStyle.h"

namespace unity {

  PreviewBasicButton::PreviewBasicButton (nux::TextureArea *image, NUX_FILE_LINE_DECL)
      : nux::Button (image, NUX_FILE_LINE_PARAM)
      , prelight_ (NULL)
      , active_ (NULL)
      , normal_ (NULL) {
    InitTheme();
  }

  PreviewBasicButton::PreviewBasicButton (const std::string label, NUX_FILE_LINE_DECL)
      : nux::Button (label, NUX_FILE_LINE_PARAM)
      , prelight_ (NULL)
      , active_ (NULL)
      , normal_ (NULL) {
    InitTheme();
  }

  PreviewBasicButton::PreviewBasicButton (const std::string label, nux::TextureArea *image, NUX_FILE_LINE_DECL)
      : nux::Button (label,  image, NUX_FILE_LINE_PARAM)
      , prelight_ (NULL)
      , active_ (NULL)
      , normal_ (NULL) {
    InitTheme();
  }

  PreviewBasicButton::PreviewBasicButton (NUX_FILE_LINE_DECL)
      : nux::Button (NUX_FILE_LINE_PARAM)
      , prelight_ (NULL)
      , active_ (NULL)
      , normal_ (NULL) {
    InitTheme();
  }

  PreviewBasicButton::~PreviewBasicButton() {
    delete prelight_;
    delete active_;
    delete normal_;
  }

  void PreviewBasicButton::InitTheme()
  {
    if (prelight_ == NULL)
    {
      prelight_ = new nux::CairoWrapper(GetGeometry(), sigc::bind(sigc::mem_fun(this, &PreviewBasicButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRELIGHT));
      active_ = new nux::CairoWrapper(GetGeometry(), sigc::bind(sigc::mem_fun(this, &PreviewBasicButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRESSED));
      normal_ = new nux::CairoWrapper(GetGeometry(), sigc::bind(sigc::mem_fun(this, &PreviewBasicButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_NORMAL));
    }
  }

  void PreviewBasicButton::RedrawTheme (nux::Geometry const& geom, cairo_t *cr, nux::ButtonVisualState faked_state)
  {
    dash::Style::Instance().Button(cr, faked_state, GetLabel());
  }

  long PreviewBasicButton::ComputeContentSize ()
  {
    long ret = nux::Button::ComputeContentSize();
    if (cached_geometry_ != GetGeometry())
    {
      prelight_->Invalidate(GetGeometry());
      active_->Invalidate(GetGeometry());
      normal_->Invalidate(GetGeometry());
    }

    cached_geometry_ = GetGeometry();
    return ret;
  }

  void PreviewBasicButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
  {
    gPainter.PaintBackground(GfxContext, GetGeometry());
    // set up our texture mode
    nux::TexCoordXForm texxform;
    texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

    // clear what is behind us
    unsigned int alpha = 0, src = 0, dest = 0;

    GfxContext.GetRenderStates().GetBlend(alpha, src, dest);
    GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    nux::Color col = nux::color::Black;
    col.alpha = 0;
    GfxContext.QRP_Color(GetGeometry().x,
                         GetGeometry().y,
                         GetGeometry().width,
                         GetGeometry().height,
                         col);

    nux::BaseTexture *texture = normal_->GetTexture();
    if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)
      texture = prelight_->GetTexture();
    else if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRESSED)
    {
      texture = active_->GetTexture();
    }

    GfxContext.QRP_1Tex(GetGeometry().x,
                        GetGeometry().y,
                        GetGeometry().width,
                        GetGeometry().height,
                        texture->GetDeviceTexture(),
                        texxform,
                        nux::Color(1.0f, 1.0f, 1.0f, 1.0f));

    GfxContext.GetRenderStates().SetBlend(alpha, src, dest);

  }

  void PreviewBasicButton::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
    //nux::Button::DrawContent(GfxContext, force_draw);
  }

  void PreviewBasicButton::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Button::PostDraw(GfxContext, force_draw);
  }

}
