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
 * Authored by: Andrea Cimitan <andrea.cimitan@canonical.com>
 *
 */

#include "ApplicationScreenshot.h"
#include "unity-shared/IntrospectableWrappers.h"
#include <NuxCore/Logger.h>
#include <Nux/VLayout.h>

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
nux::logging::Logger logger("unity.dash.previews.applicationscreenshot");
}

NUX_IMPLEMENT_OBJECT_TYPE(ApplicationScreenshot);

ApplicationScreenshot::ApplicationScreenshot()
  : View(NUX_TRACKER_LOCATION)
  , overlay_text_(nullptr)
{
  SetupViews();
}

ApplicationScreenshot::~ApplicationScreenshot()
{
  if (overlay_text_)
    overlay_text_->UnReference();
}

void ApplicationScreenshot::SetImage(std::string const& image_hint)
{
  if (overlay_text_ && GetLayout())
    GetLayout()->RemoveChildObject(overlay_text_);

  texture_screenshot_.Adopt(nux::CreateTexture2DFromFile(image_hint.c_str(), -1, true));  

  if (!texture_screenshot_ && GetLayout())
  {
    GetLayout()->AddView(overlay_text_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 100.0, nux::LayoutPosition(1));   
  }
  QueueDraw();
}

void ApplicationScreenshot::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  gfx_engine.PushClippingRectangle(base);
  nux::GetPainter().PaintBackground(gfx_engine, base);

  if (texture_screenshot_)
  {
    nux::Geometry imageDest = base;
    nux::TexCoordXForm texxform;

    float base_apsect = float(base.GetWidth()) / base.GetHeight();
    float image_aspect = float(texture_screenshot_->GetWidth()) / texture_screenshot_->GetHeight();

    if (image_aspect > base_apsect)
    {
      imageDest.SetHeight(float(imageDest.GetWidth())/image_aspect);
    } 
    if (image_aspect < base_apsect)
    {
      imageDest.SetWidth(image_aspect*imageDest.GetHeight());
    }

    int border_width = 1;

    gfx_engine.QRP_Color(imageDest.x + (float(base.GetWidth() - imageDest.GetWidth()) / 2),
                      imageDest.y + (float(base.GetHeight() - imageDest.GetHeight()) / 2),
                      imageDest.GetWidth(),
                      imageDest.GetHeight(),
                      nux::Color(0.15f, 0.15f, 0.15f));

    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_SCALE_COORD);
    texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);
    texxform.SetFilter(nux::TEXFILTER_LINEAR, nux::TEXFILTER_LINEAR);

    texxform.u0 = 0;
    texxform.v0 = 0;
    texxform.u1 = imageDest.width;
    texxform.v1 = imageDest.height;

    gfx_engine.QRP_1Tex(imageDest.x + (float(base.GetWidth() - imageDest.GetWidth()) / 2) + border_width,
                        imageDest.y + (float(base.GetHeight() - imageDest.GetHeight()) / 2) + border_width,
                        imageDest.width - (border_width * 2),
                        imageDest.height - (border_width * 2),
                        texture_screenshot_.GetPointer()->GetDeviceTexture(),
                        texxform,
                        nux::color::White);
  }
  else
  {
    unsigned int alpha, src, dest = 0;
    gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
    gfx_engine.GetRenderStates().SetBlend(true);

    gfx_engine.QRP_Color(base.x,
                      base.y,
                      base.GetWidth(),
                      base.GetHeight(),
                      nux::Color(0.15f, 0.15f, 0.15f, 0.15f));

    gPainter.Paint2DQuadWireframe(gfx_engine,
                      base.x+1,
                      base.y,
                      base.GetWidth(),
                      base.GetHeight(),
                      nux::Color(0.5f, 0.5, 0.5, 0.15f));

    gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);
  }

  gfx_engine.PopClippingRectangle();
}

void ApplicationScreenshot::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

  if (GetLayout())
    GetLayout()->ProcessDraw(gfx_engine, force_draw);

  gfx_engine.PopClippingRectangle();
}

std::string ApplicationScreenshot::GetName() const
{
  return "ApplicationScreenshot";
}

void ApplicationScreenshot::SetupViews()
{
  nux::VLayout* layout = new nux::VLayout();
  layout->AddSpace(0, 1);
  layout->AddSpace(0, 1);
  SetLayout(layout);

  overlay_text_ = new nux::StaticCairoText("", NUX_TRACKER_LOCATION);
  overlay_text_->Reference();
  overlay_text_->SetTextAlignment(nux::StaticCairoText::NUX_ALIGN_CENTRE);
  overlay_text_->SetFont("Ubuntu 14");
  overlay_text_->SetLines(-3);
  overlay_text_->SetText("No Image Available");
}

void ApplicationScreenshot::SetFont(std::string const& font)
{
  overlay_text_->SetFont(font);
}

}
}
}