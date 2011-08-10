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
#include "config.h"
#include "Nux/Nux.h"
#include "FilterBasicButton.h"

namespace unity {

  FilterBasicButton::FilterBasicButton (nux::TextureArea *image, NUX_FILE_LINE_DECL)
      : nux::ToggleButton (image, NUX_FILE_LINE_PARAM)
      , prelight_ (NULL)
      , active_ (NULL)
      , normal_ (NULL)
  {
    InitTheme();
  }

  FilterBasicButton::FilterBasicButton (const std::string label, NUX_FILE_LINE_DECL)
      : nux::ToggleButton (label, NUX_FILE_LINE_PARAM)
      , prelight_ (NULL)
      , active_ (NULL)
      , normal_ (NULL)
  {
    InitTheme();
  }

  FilterBasicButton::FilterBasicButton (const std::string label, nux::TextureArea *image, NUX_FILE_LINE_DECL)
      : nux::ToggleButton (label,  image, NUX_FILE_LINE_PARAM)
      , prelight_ (NULL)
      , active_ (NULL)
      , normal_ (NULL)
  {
    InitTheme();
  }

  FilterBasicButton::FilterBasicButton (NUX_FILE_LINE_DECL)
      : nux::ToggleButton (NUX_FILE_LINE_PARAM)
      , prelight_ (NULL)
      , active_ (NULL)
      , normal_ (NULL)
  {
    InitTheme();
  }

  FilterBasicButton::~FilterBasicButton() {
   delete prelight_;
    delete active_;
    delete normal_;

  }

  void FilterBasicButton::InitTheme()
  {
    if (prelight_ == NULL)
    {
      prelight_ = new nux::CairoWrapper(GetGeometry(), sigc::bind(sigc::mem_fun(this, &FilterBasicButton::RedrawTheme), nux::State::NUX_STATE_PRELIGHT));
      active_ = new nux::CairoWrapper(GetGeometry(), sigc::bind(sigc::mem_fun(this, &FilterBasicButton::RedrawTheme), nux::State::NUX_STATE_ACTIVE));
      normal_ = new nux::CairoWrapper(GetGeometry(), sigc::bind(sigc::mem_fun(this, &FilterBasicButton::RedrawTheme), nux::State::NUX_STATE_NORMAL));
    }

    SetMinimumHeight(32);
  }

  void FilterBasicButton::RedrawTheme (nux::Geometry const& geom, cairo_t *cr, nux::State faked_state)
  {
    DashStyle *dash_style = DashStyle::GetDefault();
    dash_style->Button (cr, faked_state, label);
  }

  long FilterBasicButton::ComputeLayout2 ()
  {
    long ret = nux::Button::ComputeLayout2();
    if (cached_geometry_ != GetGeometry())
    {
      prelight_->Invalidate(GetGeometry());
      active_->Invalidate(GetGeometry());
      normal_->Invalidate(GetGeometry());
    }

    cached_geometry_ = GetGeometry();
    return ret;
  }

  long int FilterBasicButton::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return nux::Button::ProcessEvent(ievent, TraverseInfo, ProcessEventInfo);
  }

  void FilterBasicButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    gPainter.PaintBackground(GfxContext, GetGeometry());
    // set up our texture mode
    nux::TexCoordXForm texxform;
    texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

    // clear what is behind us
    nux::t_u32 alpha = 0, src = 0, dest = 0;
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
    if (state == nux::State::NUX_STATE_PRELIGHT)
      texture = prelight_->GetTexture();
    else if (state == nux::State::NUX_STATE_ACTIVE)
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

  void FilterBasicButton::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
  }

  void FilterBasicButton::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Button::PostDraw(GfxContext, force_draw);
  }

}
