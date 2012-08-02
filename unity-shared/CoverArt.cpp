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
 *              Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#include "CoverArt.h"
#include "unity-shared/IntrospectableWrappers.h"
#include <NuxCore/Logger.h>
#include <Nux/VLayout.h>
#include "ThumbnailGenerator.h"
#include "DashStyle.h"
#include "IconLoader.h"

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
nux::logging::Logger logger("unity.dash.previews.coverart");
}

NUX_IMPLEMENT_OBJECT_TYPE(CoverArt);

CoverArt::CoverArt()
  : View(NUX_TRACKER_LOCATION)
  , overlay_text_(nullptr)
  , thumb_handle_(0)
  , slot_handle_(0)
  , stretch_image_(false)
  , waiting_(false)
  , rotation_(0.0)
{
  SetupViews();

  ThumbnailGenerator::Instance().ready.connect(sigc::mem_fun(this, &CoverArt::OnThumbnailGenerated));
  ThumbnailGenerator::Instance().error.connect(sigc::mem_fun(this, &CoverArt::OnThumbnailError));
}

CoverArt::~CoverArt()
{
  if (overlay_text_)
    overlay_text_->UnReference();

  if (slot_handle_ > 0)
  {
    IconLoader::GetDefault().DisconnectHandle(slot_handle_);
    slot_handle_ = 0;
  }
}

void CoverArt::SetImage(std::string const& image_hint)
{
  if (slot_handle_ > 0)
  {
    IconLoader::GetDefault().DisconnectHandle(slot_handle_);
    slot_handle_ = 0;
  }

  if (GetLayout())
    GetLayout()->RemoveChildObject(overlay_text_);

  GIcon*  icon = g_icon_new_for_string(image_hint.c_str(), NULL);

  if (g_strrstr(image_hint.c_str(), "://"))
  {
    // for files on disk, we stretch to maximum aspect ratio.
    stretch_image_  = true;
    spinner_timeout_.reset();
    waiting_ = false;

    GFileInputStream       *stream;
    GError                 *error = NULL;
    GFile                  *file;

    /* try to open the source file for reading */
    file = g_file_new_for_uri (image_hint.c_str());
    stream = g_file_read (file, NULL, &error);
    g_object_unref (file);

    if (error != NULL)
    {
      g_error_free (error);
      QueueDraw();
      return;
    }

    /* stream image into pixel-buffer. */
    glib::Object<GdkPixbuf> pixbuf(gdk_pixbuf_new_from_stream (G_INPUT_STREAM (stream), NULL, &error));
    g_object_unref (stream);

    texture_screenshot_.Adopt(nux::CreateTexture2DFromPixbuf(pixbuf, true));
    g_object_unref(pixbuf);

    if (!texture_screenshot_ && GetLayout())
    {
      GetLayout()->AddView(overlay_text_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 100.0, nux::LayoutPosition(1));   
      ComputeContentSize();
    }
    QueueDraw();
  }
  else
  {
    GFile *file = g_file_new_for_path (image_hint.c_str());
    if (g_file_query_exists(file, NULL))
    {
      // for files on disk, we stretch to maximum aspect ratio.
      stretch_image_  = true;
      spinner_timeout_.reset();
      waiting_ = false;

      GFileInputStream       *stream;
      GError                 *error = NULL;

      /* try to open the source file for reading */
      stream = g_file_read (file, NULL, &error);
      g_object_unref (file);

      if (error != NULL)
      {
        g_error_free (error);
        QueueDraw();
        return;
      }

      /* stream image into pixel-buffer. */
      glib::Object<GdkPixbuf> pixbuf(gdk_pixbuf_new_from_stream (G_INPUT_STREAM (stream), NULL, &error));
      g_object_unref (stream);

      texture_screenshot_.Adopt(nux::CreateTexture2DFromPixbuf(pixbuf, true));

      if (!texture_screenshot_ && GetLayout())
      {
        GetLayout()->AddView(overlay_text_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 100.0, nux::LayoutPosition(1));   
        ComputeContentSize();
      }
      QueueDraw();
    }
    else if (G_IS_ICON(icon))
    {
      StartWaiting();
      slot_handle_ = IconLoader::GetDefault().LoadFromGIconString(image_hint, 128, sigc::mem_fun(this, &CoverArt::IconLoaded));
    }
    else
    {
      StartWaiting();
      slot_handle_ = IconLoader::GetDefault().LoadFromIconName(image_hint, 128, sigc::mem_fun(this, &CoverArt::IconLoaded));
    }
  }

  if (icon != NULL)
    g_object_unref(icon);
}

void CoverArt::GenerateImage(std::string const& uri)
{
  StartWaiting();
  thumb_handle_ = ThumbnailGenerator::Instance().GetThumbnail(uri, 512);
}

void CoverArt::StartWaiting()
{
  waiting_ = true;

  rotate_matrix_.Rotate_z(0.0f);
  rotation_ = 0.0f;

  spinner_timeout_.reset(new glib::TimeoutSeconds(5, [&]
  {
    texture_screenshot_.Release();
    waiting_ = false;

    if (GetLayout())
    {
      GetLayout()->RemoveChildObject(overlay_text_);
      GetLayout()->AddView(overlay_text_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 100.0, nux::LayoutPosition(1));
      ComputeContentSize();
    }

    QueueDraw();
    return false;
  }));
  
  QueueDraw();
}

void CoverArt::IconLoaded(std::string const& texid, unsigned size, glib::Object<GdkPixbuf> const& pixbuf)
{
  if (GetLayout())
    GetLayout()->RemoveChildObject(overlay_text_);

  slot_handle_ = 0;
  stretch_image_ = false;
  texture_screenshot_.Release();
  waiting_ = false;
  spinner_timeout_.reset();

  if (pixbuf)
  {
    texture_screenshot_.Adopt(nux::CreateTextureFromPixbuf(pixbuf));
  
    QueueDraw();
  }
  else if (GetLayout())
  {
    GetLayout()->AddView(overlay_text_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 100.0, nux::LayoutPosition(1));
    ComputeContentSize();
  }
}

void CoverArt::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  gfx_engine.PushClippingRectangle(base);
  nux::GetPainter().PaintBackground(gfx_engine, base);

  if (texture_screenshot_)
  {
    nux::Geometry imageDest = base;
    nux::TexCoordXForm texxform;

    gfx_engine.QRP_Color(base.x,
                      base.y,
                      base.GetWidth(),
                      base.GetHeight(),
                      nux::Color(0.03f, 0.03f, 0.03f, 0.0f));

    if (stretch_image_ || base.GetWidth() < texture_screenshot_->GetWidth() || base.height < texture_screenshot_->GetHeight())
    {
      float base_apsect = float(base.GetWidth()) / base.GetHeight();
      float image_aspect = float(texture_screenshot_->GetWidth()) / texture_screenshot_->GetHeight();

      if (image_aspect > base_apsect)
      {
        imageDest.SetHeight(float(imageDest.GetWidth()) / image_aspect);
      } 
      if (image_aspect < base_apsect)
      {
        imageDest.SetWidth(image_aspect * imageDest.GetHeight());
      }
    }
    else
    {
      imageDest = nux::Geometry(0, 0, texture_screenshot_->GetWidth(), texture_screenshot_->GetHeight());
    }


    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_SCALE_COORD);
    texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);
    texxform.SetFilter(nux::TEXFILTER_LINEAR, nux::TEXFILTER_LINEAR);

    texxform.u0 = 0;
    texxform.v0 = 0;
    texxform.u1 = imageDest.width;
    texxform.v1 = imageDest.height;

    int border_width = 1;
    gfx_engine.QRP_1Tex(base.x + (float(base.GetWidth() - imageDest.GetWidth()) / 2) + border_width,
                        base.y + (float(base.GetHeight() - imageDest.GetHeight()) / 2) + border_width,
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
                      nux::Color(0.03f, 0.03f, 0.03f, 0.0f));

    gPainter.Paint2DQuadWireframe(gfx_engine,
                      base.x+1,
                      base.y,
                      base.GetWidth(),
                      base.GetHeight(),
                      nux::Color(0.5f, 0.5, 0.5, 0.15f));

    if (waiting_)
    {  
      nux::TexCoordXForm texxform;
      texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
      texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
      texxform.min_filter = nux::TEXFILTER_LINEAR;
      texxform.mag_filter = nux::TEXFILTER_LINEAR;

      nux::Geometry spin_geo(base.x + ((base.width - spin_->GetWidth()) / 2),
                             base.y + ((base.height - spin_->GetHeight()) / 2),
                             spin_->GetWidth(),
                             spin_->GetHeight());
      // Geometry (== Rect) uses integers which were rounded above,
      // hence an extra 0.5 offset for odd sizes is needed
      // because pure floating point is not being used.
      int spin_offset_w = !(base.width % 2) ? 0 : 1;
      int spin_offset_h = !(base.height % 2) ? 0 : 1;

      gfx_engine.PushModelViewMatrix(nux::Matrix4::TRANSLATE(-spin_geo.x - (spin_geo.width + spin_offset_w) / 2.0f,
                                                             -spin_geo.y - (spin_geo.height + spin_offset_h) / 2.0f, 0));
      gfx_engine.PushModelViewMatrix(rotate_matrix_);
      gfx_engine.PushModelViewMatrix(nux::Matrix4::TRANSLATE(spin_geo.x + (spin_geo.width + spin_offset_w) / 2.0f,
                                                             spin_geo.y + (spin_geo.height + spin_offset_h) / 2.0f, 0));

      gfx_engine.QRP_1Tex(spin_geo.x,
                          spin_geo.y,
                          spin_geo.width,
                          spin_geo.height,
                          spin_->GetDeviceTexture(),
                          texxform,
                          nux::color::White);

      gfx_engine.PopModelViewMatrix();
      gfx_engine.PopModelViewMatrix();
      gfx_engine.PopModelViewMatrix();

      if (!frame_timeout_)
      {
        frame_timeout_.reset(new glib::Timeout(22, sigc::mem_fun(this, &CoverArt::OnFrameTimeout)));
      }
    }

    gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);
  }

  gfx_engine.PopClippingRectangle();
}

void CoverArt::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

  if (GetLayout())
    GetLayout()->ProcessDraw(gfx_engine, force_draw);

  gfx_engine.PopClippingRectangle();
}

std::string CoverArt::GetName() const
{
  return "CoverArt";
}

void CoverArt::SetupViews()
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

  dash::Style& style = dash::Style::Instance();
  spin_ = style.GetSearchSpinIcon();

  rotate_matrix_.Identity();
  rotate_matrix_.Rotate_z(0.0);

}

void CoverArt::SetFont(std::string const& font)
{
  overlay_text_->SetFont(font);
}

void CoverArt::OnThumbnailGenerated(unsigned int thumb_handle, std::string const& uri)
{
  if (thumb_handle_ != thumb_handle)
    return;

  SetImage(uri);
}

void CoverArt::OnThumbnailError(unsigned int thumb_handle, std::string const& error_hint)
{
  if (thumb_handle_ != thumb_handle)
    return;

  LOG_WARNING(logger) << "Failed to generate thumbnail: " << error_hint;
  spinner_timeout_.reset();
  frame_timeout_.reset();
  waiting_ = false;

  texture_screenshot_.Release();
  if (GetLayout())
  {
    GetLayout()->RemoveChildObject(overlay_text_);
    GetLayout()->AddView(overlay_text_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 100.0, nux::LayoutPosition(1));   
    ComputeContentSize();
  }
  QueueDraw();
}


bool CoverArt::OnFrameTimeout()
{
  rotation_ += 0.1f;

  if (rotation_ >= 360.0f)
    rotation_ = 0.0f;

  rotate_matrix_.Rotate_z(rotation_);
  QueueDraw();

  frame_timeout_.reset();
  return false;
}

}
}
}