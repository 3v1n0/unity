// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jay Taoko <jay.taoko@canonical.com>
 */

#include "BackgroundEffectHelper.h"

using namespace unity;

std::list<BackgroundEffectHelper*> BackgroundEffectHelper::registered_list_;

nux::Property<BlurType> BackgroundEffectHelper::blur_type (BLUR_ACTIVE);
nux::Property<float> BackgroundEffectHelper::sigma_high (5.0f);
nux::Property<float> BackgroundEffectHelper::sigma_med  (4.0f);
nux::Property<float> BackgroundEffectHelper::sigma_low  (1.0f);
nux::Property<bool> BackgroundEffectHelper::updates_enabled (true);


BackgroundEffectHelper::BackgroundEffectHelper()
{
  noise_texture_ = nux::CreateTextureFromFile(PKGDATADIR"/dash_noise.png");
  Register(this);
}

BackgroundEffectHelper::~BackgroundEffectHelper()
{
  noise_texture_->UnReference();
  Unregister(this);
}

void BackgroundEffectHelper::QueueDrawOnOwners ()
{
  for (BackgroundEffectHelper* helper : registered_list_)
  {
    nux::View *owner = helper->owner();
    if (owner)
      owner->QueueDraw();
  }
}

void BackgroundEffectHelper::Register   (BackgroundEffectHelper *self)
{
  registered_list_.push_back(self);
}

void BackgroundEffectHelper::Unregister (BackgroundEffectHelper *self)
{
  registered_list_.remove(self);
}

void BackgroundEffectHelper::DirtyCache ()
{
  if (blur_texture_.IsValid ())
    blur_texture_.Release ();
}

nux::ObjectPtr<nux::IOpenGLBaseTexture> BackgroundEffectHelper::GetBlurRegion(nux::Geometry geo, bool force_update)
{
  nux::GraphicsEngine* graphics_engine = nux::GetGraphicsDisplay()->GetGraphicsEngine();

  bool should_update = updates_enabled() || force_update;

  if ((blur_type != BLUR_ACTIVE || !should_update) 
      && blur_texture_.IsValid() 
      && (geo == blur_geometry_))
  {
    return blur_texture_;
  }

  blur_geometry_ =  nux::Geometry(0, 0, graphics_engine->GetWindowWidth(), graphics_engine->GetWindowHeight()).Intersect(geo);

  if (blur_geometry_.IsNull() || blur_type == BLUR_NONE || !nux::GetGraphicsDisplay()->GetGpuDevice()->backup_texture0_.IsValid())
  {
    return nux::ObjectPtr<nux::IOpenGLBaseTexture>(NULL);
  }

  // save the current fbo
  nux::ObjectPtr<nux::IOpenGLFrameBufferObject> current_fbo = nux::GetGraphicsDisplay()->GetGpuDevice()->GetCurrentFrameBufferObject();
  nux::GetGraphicsDisplay ()->GetGpuDevice ()->DeactivateFrameBuffer ();
  
  // Set a viewport to the requested size
  graphics_engine->SetViewport (0, 0, blur_geometry_.width, blur_geometry_.height);
  graphics_engine->SetScissor (0, 0, blur_geometry_.width, blur_geometry_.height);
  // Disable nux scissoring
  graphics_engine->GetRenderStates ().EnableScissor (false);

  // The background texture is the same size as the monitor where we are rendering.
  nux::TexCoordXForm texxform__bg;
  texxform__bg.flip_v_coord = false;
  texxform__bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform__bg.uoffset = ((float) blur_geometry_.x) / graphics_engine->GetWindowWidth ();
  texxform__bg.voffset = ((float) graphics_engine->GetWindowHeight () - blur_geometry_.y - blur_geometry_.height) / graphics_engine->GetWindowHeight ();

  int opengl_version = nux::GetGraphicsDisplay()->GetGpuDevice()->GetOpenGLMajorVersion();
  bool support_frag = nux::GetGraphicsDisplay()->GetGpuDevice()->GetGpuInfo().Support_ARB_Fragment_Shader();
  bool support_vert = nux::GetGraphicsDisplay()->GetGpuDevice()->GetGpuInfo().Support_ARB_Vertex_Shader();

  if (support_vert && support_frag && opengl_version >= 2)
  {
    float noise_factor = 1.2f;
    float gaussian_sigma = opengl_version >= 3 ? sigma_high : sigma_med;
    int blur_passes = 1;

    nux::ObjectPtr<nux::IOpenGLBaseTexture> device_texture = nux::GetGraphicsDisplay()->GetGpuDevice()->backup_texture0_;
    nux::ObjectPtr<nux::CachedBaseTexture> noise_device_texture = graphics_engine->CacheResource(noise_texture_);

    int down_size_factor = 1;
    unsigned int buffer_width = blur_geometry_.width;
    unsigned int buffer_height = blur_geometry_.height;

    int x =  0;
    int y =  0;

    unsigned int down_size_width = buffer_width / down_size_factor;
    unsigned int down_size_height = buffer_height / down_size_factor;

    nux::TexCoordXForm texxform;
    nux::TexCoordXForm noise_texxform;

    texxform.SetFilter(nux::TEXFILTER_LINEAR, nux::TEXFILTER_LINEAR);
    
    noise_texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    noise_texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
    noise_texxform.SetFilter(nux::TEXFILTER_NEAREST, nux::TEXFILTER_NEAREST);

    // Down size
    graphics_engine->QRP_GetCopyTexture(down_size_width, down_size_height, temp_device_texture0_,
     device_texture, texxform__bg, nux::color::White);

    // Blur at a lower resolution (less pixels to process)
    temp_device_texture1_ = graphics_engine->QRP_GetHQBlur(x, y, down_size_width, down_size_height,
     temp_device_texture0_, texxform, nux::color::White,
     gaussian_sigma, blur_passes);

    // Up size
    graphics_engine->QRP_GetCopyTexture(buffer_width, buffer_height, temp_device_texture0_,
     temp_device_texture1_, texxform, nux::color::White);

    // Add Noise
    blur_texture_ = graphics_engine->QRP_GLSL_GetDisturbedTexture(
      0, 0, buffer_width, buffer_height,
      noise_device_texture->m_Texture, noise_texxform, nux::Color (
      noise_factor * 1.0f/buffer_width,
      noise_factor * 1.0f/buffer_height, 1.0f, 1.0f),
      temp_device_texture0_, texxform, nux::color::White);
  }
  else
  {
    // GPUs with only ARB support are treated here

    float gaussian_sigma = sigma_low;
    int blur_passes = 1;

    nux::ObjectPtr<nux::IOpenGLBaseTexture> device_texture = nux::GetGraphicsDisplay()->GetGpuDevice()->backup_texture0_;
    nux::ObjectPtr<nux::CachedBaseTexture> noise_device_texture = graphics_engine->CacheResource(noise_texture_);

    unsigned int offset = 0;
    int quad_width = blur_geometry_.width;
    int quad_height = blur_geometry_.height;

    int down_size_factor = 4;
    unsigned int buffer_width = quad_width + 2 * offset;
    unsigned int buffer_height = quad_height + 2 * offset;

    int x = (buffer_width - quad_width) / 2;
    int y = (buffer_height - quad_height) / 2;

    unsigned int down_size_width = buffer_width / down_size_factor;
    unsigned int down_size_height = buffer_height / down_size_factor;

    nux::TexCoordXForm texxform;
    nux::TexCoordXForm noise_texxform;

    texxform.SetFilter(nux::TEXFILTER_LINEAR, nux::TEXFILTER_LINEAR);

    noise_texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    noise_texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
    noise_texxform.SetFilter(nux::TEXFILTER_NEAREST, nux::TEXFILTER_NEAREST);

    // Copy source texture
    graphics_engine->QRP_GetCopyTexture(buffer_width, buffer_height, temp_device_texture0_,
     device_texture, texxform__bg, nux::color::White);

    // Down size
    graphics_engine->QRP_GetCopyTexture(down_size_width, down_size_height, ds_temp_device_texture1_,
     temp_device_texture0_, texxform, nux::color::White);

    // Blur at a lower resolution (less pixels to process)
    ds_temp_device_texture0_ = graphics_engine->QRP_GetBlurTexture(x, y, down_size_width, down_size_height,
     ds_temp_device_texture1_, texxform, nux::color::White,
     gaussian_sigma, blur_passes);

    // // Copy to new texture
    // graphics_engine->QRP_GetCopyTexture(down_size_width, down_size_height, temp_device_texture1_, temp_device_texture0_, texxform, nux::color::White);

    // Up size
    graphics_engine->QRP_GetCopyTexture(buffer_width, buffer_height, temp_device_texture1_,
     ds_temp_device_texture0_, texxform, nux::color::White);

    blur_texture_ = temp_device_texture1_;      
  }

  if (current_fbo.IsValid())
  {
    current_fbo->Activate(true);
    graphics_engine->Push2DWindow(current_fbo->GetWidth(), current_fbo->GetHeight());
    graphics_engine->GetRenderStates ().EnableScissor (true);
  }
  else
  {
    graphics_engine->SetViewport(0, 0, graphics_engine->GetWindowWidth(), graphics_engine->GetWindowHeight());
    graphics_engine->Push2DWindow(graphics_engine->GetWindowWidth(), graphics_engine->GetWindowHeight());
    graphics_engine->ApplyClippingRectangle();
  }

  return blur_texture_;
}
