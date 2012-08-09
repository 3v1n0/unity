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
#include "unity-shared/CairoTexture.h"
#include <NuxCore/Logger.h>
#include <Nux/VLayout.h>
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

const int icon_width = 256;
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

  if (notifier_)
    notifier_->Cancel();
}

void CoverArt::SetImage(std::string const& image_hint)
{ 
  spinner_timeout_.reset();
  frame_timeout_.reset();
  waiting_ = false;

  if (slot_handle_ > 0)
  {
    IconLoader::GetDefault().DisconnectHandle(slot_handle_);
    slot_handle_ = 0;
  }

  if (GetLayout())
    GetLayout()->RemoveChildObject(overlay_text_);

  GIcon* icon = g_icon_new_for_string(image_hint.c_str(), NULL);

  glib::Object<GFile> image_file;
  if (g_strrstr(image_hint.c_str(), "://"))
  {
    /* try to open the source file for reading */
    image_file = g_file_new_for_uri (image_hint.c_str());
  }
  else
  {
    image_file = g_file_new_for_path (image_hint.c_str());
  }

  if (g_file_query_exists(image_file, NULL))
  {
    // for files on disk, we stretch to maximum aspect ratio.
    stretch_image_  = true;

    GFileInputStream       *stream;
    GError                 *error = NULL;

    stream = g_file_read (image_file, NULL, &error);

    if (error != NULL)
    {
      g_error_free (error);

      if (icon != NULL)
        g_object_unref(icon);
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
    slot_handle_ = IconLoader::GetDefault().LoadFromGIconString(image_hint, icon_width, sigc::mem_fun(this, &CoverArt::IconLoaded));
  }
  else
  {
    StartWaiting();
    slot_handle_ = IconLoader::GetDefault().LoadFromIconName(image_hint, icon_width, sigc::mem_fun(this, &CoverArt::IconLoaded));
  }

  if (icon != NULL)
    g_object_unref(icon);
}

void CoverArt::GenerateImage(std::string const& uri)
{
  StartWaiting();
  notifier_ = ThumbnailGenerator::Instance().GetThumbnail(uri, 512);

  notifier_->ready.connect(sigc::mem_fun(this, &CoverArt::OnThumbnailGenerated));
  notifier_->error.connect(sigc::mem_fun(this, &CoverArt::OnThumbnailError));
}

void CoverArt::StartWaiting()
{
  if (waiting_)
    return;

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
  // Finished waiting
  spinner_timeout_.reset();
  frame_timeout_.reset();
  waiting_ = false;
  
  stretch_image_ = false;

  int height = size;

  int pixbuf_width, pixbuf_height;
  pixbuf_width = gdk_pixbuf_get_width(pixbuf);
  pixbuf_height = gdk_pixbuf_get_height(pixbuf);
  if (G_UNLIKELY(!pixbuf_height || !pixbuf_width))
  {
    pixbuf_width = (pixbuf_width) ? pixbuf_width : 1; // no zeros please
    pixbuf_height = (pixbuf_height) ? pixbuf_height: 1; // no zeros please
  }

  if (pixbuf_width == pixbuf_height)
  {
    // quick path for square icons
    texture_screenshot_.Adopt(nux::CreateTexture2DFromPixbuf(pixbuf, true));
    QueueDraw();
  }
  else
  {
    // slow path for non square icons that must be resized to fit in the square
    // texture

    float aspect = static_cast<float>(pixbuf_height) / pixbuf_width; // already sanitized width/height so can not be 0.0
    if (aspect < 1.0f)
    {
      pixbuf_width = icon_width;
      pixbuf_height = pixbuf_width * aspect;

      if (pixbuf_height > height)
      {
        // scaled too big, scale down
        pixbuf_height = height;
        pixbuf_width = pixbuf_height / aspect;
      }
    }
    else
    {
      pixbuf_height = height;
      pixbuf_width = pixbuf_height / aspect;
    }

    if (gdk_pixbuf_get_height(pixbuf) == pixbuf_height)
    {
      // we changed our mind, fast path is good
      texture_screenshot_.Adopt(nux::CreateTexture2DFromPixbuf(pixbuf, true));
      QueueDraw();
      return;
    }

    nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, pixbuf_width, pixbuf_height);
    cairo_t* cr = cairo_graphics.GetInternalContext();

    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);

    float scale = float(pixbuf_height) / gdk_pixbuf_get_height(pixbuf);

    //cairo_translate(cr,
    //                static_cast<int>((width - (pixbuf_width * scale)) * 0.5),
    //                static_cast<int>((height - (pixbuf_height * scale)) * 0.5));

    cairo_scale(cr, scale, scale);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
    cairo_paint(cr);

    texture_screenshot_.Adopt(texture_from_cairo_graphics(cairo_graphics));
    QueueDraw();
  }
}

void CoverArt::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  gfx_engine.PushClippingRectangle(base);
  nux::GetPainter().PaintBackground(gfx_engine, base);

  unsigned int alpha, src, dest = 0;
  gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
  gfx_engine.GetRenderStates().SetBlend(true);

    gfx_engine.QRP_Color(base.x,
                      base.y,
                      base.GetWidth(),
                      base.GetHeight(),
                      nux::Color(0.03f, 0.03f, 0.03f, 0.0f));

  if (texture_screenshot_)
  {
    nux::Geometry imageDest = base;
    nux::TexCoordXForm texxform;

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

    gfx_engine.QRP_1Tex(base.x + (float(base.GetWidth() - imageDest.GetWidth()) / 2),
                        base.y + (float(base.GetHeight() - imageDest.GetHeight()) / 2),
                        imageDest.width,
                        imageDest.height,
                        texture_screenshot_.GetPointer()->GetDeviceTexture(),
                        texxform,
                        nux::color::White);
  }
  else
  {
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
  }
  
  gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);

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

void CoverArt::OnThumbnailGenerated(std::string const& uri)
{
  SetImage(uri);
  notifier_.Release();
}

void CoverArt::OnThumbnailError(std::string const& error_hint)
{
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
  notifier_.Release();
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