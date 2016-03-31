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

#include <NuxCore/Logger.h>
#include "BackgroundEffectHelper.h"

#include "TextureCache.h"
#include "UnitySettings.h"

namespace
{
DECLARE_LOGGER(logger, "unity.background_effect_helper");

const int BLUR_RADIUS = 3;
const float sigma_high = 5.0f;
const float sigma_med = 3.0f;
const float sigma_low = 1.0f;
}

using namespace unity;

nux::Property<BlurType> BackgroundEffectHelper::blur_type(BLUR_ACTIVE);
nux::Geometry BackgroundEffectHelper::monitor_rect_;
std::list<BackgroundEffectHelper*> BackgroundEffectHelper::registered_list_;
std::vector<nux::Geometry> BackgroundEffectHelper::blur_geometries_;
sigc::signal<void, nux::Geometry const&> BackgroundEffectHelper::blur_region_needs_update_;

BackgroundEffectHelper::BackgroundEffectHelper(nux::View* view)
  : owner(view)
  , enabled(false)
  , cache_dirty(true)
{
  enabled.changed.connect(sigc::mem_fun(this, &BackgroundEffectHelper::OnEnabledChanged));
  owner.changed.connect(sigc::mem_fun(this, &BackgroundEffectHelper::OnOwnerChanged));
  TextureCache::GetDefault().themed_invalidated.connect(sigc::mem_fun(this, &BackgroundEffectHelper::LoadTextures));
  LoadTextures();

  if (Settings::Instance().GetLowGfxMode())
    blur_type = BLUR_NONE;
}

BackgroundEffectHelper::BackgroundEffectHelper()
  : BackgroundEffectHelper(nullptr)
{}

BackgroundEffectHelper::~BackgroundEffectHelper()
{
  Unregister(this);
}

void BackgroundEffectHelper::LoadTextures()
{
  noise_texture_ = TextureCache::GetDefault().FindTexture("dash_noise");
}

void BackgroundEffectHelper::OnEnabledChanged(bool enabled)
{
  if (enabled)
  {
    Register(this);
    SetupOwner(owner());
  }
  else
  {
    connections_.Clear();
    Unregister(this);
  }
}

void BackgroundEffectHelper::OnOwnerChanged(nux::View* view)
{
  geo_getter_func_ = nullptr;

  if (!view)
  {
    connections_.Clear();
    Unregister(this);
  }
}

void BackgroundEffectHelper::SetupOwner(nux::View* view)
{
  if (!view)
    return;

  auto cb = [this] (nux::Area*, nux::Geometry&) { UpdateOwnerGeometry(); };
  connections_.Add(view->geometry_changed.connect(cb));

  auto* parent = view->GetTopLevelViewWindow();
  if (!parent)
  {
    LOG_ERROR(logger) << "The parent window for the owner must be set!";
  }
  else
  {
    connections_.Add(parent->geometry_changed.connect(cb));
  }

  if (!UpdateOwnerGeometry())
  {
    DirtyCache();
    UpdateBlurGeometries();
  }
}

void BackgroundEffectHelper::SetGeometryGetter(GeometryGetterFunc const& func)
{
  geo_getter_func_ = func;
}

bool BackgroundEffectHelper::UpdateOwnerGeometry()
{
  auto const& geo = geo_getter_func_ ? geo_getter_func_() : owner()->GetAbsoluteGeometry();

  if (requested_blur_geometry_ != geo)
  {
    // For some reason a view might have this size at show time, let's just ignore them.
    if (geo.width != 1 && geo.height != 1)
    {
      requested_blur_geometry_ = geo;

      DirtyCache();
      UpdateBlurGeometries();

      return true;
    }
  }

  return false;
}

void BackgroundEffectHelper::UpdateBlurGeometries()
{
  int radius = GetBlurRadius();
  blur_geometries_.clear();
  blur_geometries_.reserve(registered_list_.size());

  for (BackgroundEffectHelper* bg_effect_helper : registered_list_)
  {
    /* Use the last requested region. The real region is clipped to the
     * monitor geometry, but that is done at paint time */
    auto const& blur_geo = bg_effect_helper->requested_blur_geometry_;

    if (!blur_geo.IsNull())
      blur_geometries_.push_back(blur_geo.GetExpand(radius, radius));
  }
}

void BackgroundEffectHelper::ProcessDamage(nux::Geometry const& geo)
{
  for (BackgroundEffectHelper* bg_effect_helper : registered_list_)
  {
    if (bg_effect_helper->cache_dirty)
      continue;

    if (geo.IsIntersecting(bg_effect_helper->requested_blur_geometry_))
    {
      bg_effect_helper->DirtyCache();
    }
  }
}

bool BackgroundEffectHelper::HasEnabledHelpers()
{
  return !registered_list_.empty();
}

float BackgroundEffectHelper::GetBlurSigma()
{
  nux::GpuDevice* gpu_device = nux::GetGraphicsDisplay()->GetGpuDevice();
  int opengl_version = gpu_device->GetOpenGLMajorVersion();
  return (opengl_version >= 3) ? sigma_high : sigma_med;
}

int BackgroundEffectHelper::GetBlurRadius()
{
  return (blur_type != BLUR_NONE) ? GetBlurSigma() * BLUR_RADIUS : 0;
}

std::vector<nux::Geometry> const& BackgroundEffectHelper::GetBlurGeometries()
{
  /* TODO: to reduce useless glCopyTexSubImage2D during paint we should return
   * here only damaged geometries */
  return blur_geometries_;
}

bool BackgroundEffectHelper::HasDirtyHelpers()
{
  for (BackgroundEffectHelper* bg_effect_helper : registered_list_)
    if (bg_effect_helper->cache_dirty)
      return true;

  return false;
}

void BackgroundEffectHelper::Register(BackgroundEffectHelper* self)
{
  auto* view = self->owner();

  if (!view)
  {
    LOG_ERROR(logger) << "Registering an invalid helper, must set an owner!";
    return;
  }

  registered_list_.push_back(self);
}

void BackgroundEffectHelper::Unregister(BackgroundEffectHelper* self)
{
  registered_list_.remove(self);
  UpdateBlurGeometries();
}

void BackgroundEffectHelper::DirtyCache()
{
  if ((cache_dirty && blur_geometry_ == requested_blur_geometry_) || !owner())
    return;

  cache_dirty = true;
  owner()->QueueDraw();

  int radius = GetBlurRadius();
  blur_region_needs_update_.emit(requested_blur_geometry_.GetExpand(radius, radius));
}

nux::ObjectPtr<nux::IOpenGLBaseTexture> BackgroundEffectHelper::GetBlurRegion(bool force_update)
{
  bool should_update = force_update || cache_dirty;

  /* Static blur: only update when the size changed */
  if ((blur_type != BLUR_ACTIVE || !should_update)
      && blur_texture_.IsValid()
      && (requested_blur_geometry_ == blur_geometry_))
  {
    return blur_texture_;
  }

  nux::GraphicsEngine* graphics_engine = nux::GetGraphicsDisplay()->GetGraphicsEngine();

  int monitor_width = BackgroundEffectHelper::monitor_rect_.width;
  int monitor_height = BackgroundEffectHelper::monitor_rect_.height;

  nux::Geometry temp = requested_blur_geometry_;
  temp.OffsetPosition(-monitor_rect_.x, -monitor_rect_.y);
  blur_geometry_ = nux::Geometry(0, 0, monitor_width, monitor_height).Intersect(temp);

  nux::GpuDevice* gpu_device = nux::GetGraphicsDisplay()->GetGpuDevice();
  if (blur_geometry_.IsNull() || blur_type == BLUR_NONE || !gpu_device->backup_texture0_.IsValid())
  {
    return nux::ObjectPtr<nux::IOpenGLBaseTexture>();
  }

  const int radius = GetBlurRadius();

  // Define a larger region of that account for the blur radius
  nux::Geometry larger_blur_geometry;
  larger_blur_geometry.x = std::max(blur_geometry_.x - radius, 0);
  larger_blur_geometry.y = std::max(blur_geometry_.y - radius, 0);

  int xx = std::min(blur_geometry_.x + blur_geometry_.width + radius, monitor_width);

  larger_blur_geometry.width = xx - larger_blur_geometry.x;

  int yy = std::min(blur_geometry_.y + blur_geometry_.height + radius, monitor_height);

  larger_blur_geometry.height = yy - larger_blur_geometry.y;

  int dleft     = blur_geometry_.x - larger_blur_geometry.x;
  int dbottom   = (larger_blur_geometry.y + larger_blur_geometry.height) - (blur_geometry_.y + blur_geometry_.height);

  // save the current fbo
  nux::ObjectPtr<nux::IOpenGLFrameBufferObject> current_fbo = gpu_device->GetCurrentFrameBufferObject();
  gpu_device->DeactivateFrameBuffer();

  // Set a viewport to the requested size
  // FIXME: We need to do multiple passes for the dirty region
  // on the underlying backup texture so that we're only updating
  // the bits that we need
  graphics_engine->SetViewport(0, 0, larger_blur_geometry.width, larger_blur_geometry.height);
  graphics_engine->SetScissor(0, 0, larger_blur_geometry.width, larger_blur_geometry.height);
  // Disable nux scissoring
  graphics_engine->GetRenderStates().EnableScissor(false);

  // The background texture is the same size as the monitor where we are rendering.
  nux::TexCoordXForm texxform__bg;
  texxform__bg.flip_v_coord = false;
  texxform__bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform__bg.uoffset = ((float) larger_blur_geometry.x) / monitor_width;
  texxform__bg.voffset = ((float) monitor_height - larger_blur_geometry.y - larger_blur_geometry.height) / monitor_height;

  bool support_frag = gpu_device->GetGpuInfo().Support_ARB_Fragment_Shader();
  bool support_vert = gpu_device->GetGpuInfo().Support_ARB_Vertex_Shader();

  if (support_vert && support_frag && gpu_device->GetOpenGLMajorVersion() >= 2)
  {
    float noise_factor = 1.1f;
    float gaussian_sigma = GetBlurSigma();

    nux::ObjectPtr<nux::IOpenGLBaseTexture> device_texture = gpu_device->backup_texture0_;
    nux::ObjectPtr<nux::CachedBaseTexture> noise_device_texture = graphics_engine->CacheResource(noise_texture_.GetPointer());

    unsigned int buffer_width = larger_blur_geometry.width;
    unsigned int buffer_height = larger_blur_geometry.height;

    blur_fx_struct_.src_texture = device_texture;
    graphics_engine->QRP_GLSL_GetLSBlurFx(0, 0, buffer_width, buffer_height,
                                          &blur_fx_struct_, texxform__bg, nux::color::White,
                                          gaussian_sigma);

    nux::TexCoordXForm texxform;
    nux::TexCoordXForm noise_texxform;

    texxform.SetFilter(nux::TEXFILTER_NEAREST, nux::TEXFILTER_NEAREST);

    noise_texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    noise_texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
    noise_texxform.SetFilter(nux::TEXFILTER_NEAREST, nux::TEXFILTER_NEAREST);

    noise_fx_struct_.src_texture = blur_fx_struct_.dst_texture;

    // Add Noise
    nux::Color noise_color(noise_factor * 1.0f/buffer_width,
                           noise_factor * 1.0f/buffer_height,
                           1.0f, 1.0f);

    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    texxform.uoffset = (blur_geometry_.x - larger_blur_geometry.x) / (float) buffer_width;
    texxform.voffset = (blur_geometry_.y - larger_blur_geometry.y) / (float) buffer_height;
    graphics_engine->QRP_GLSL_GetDisturbedTextureFx(
      0, 0, blur_geometry_.width, blur_geometry_.height,
      noise_device_texture->m_Texture, noise_texxform, noise_color,
      &noise_fx_struct_, texxform, nux::color::White);

    blur_texture_ = noise_fx_struct_.dst_texture;
  }
  else
  {
    // GPUs with only ARB support are treated here

    float gaussian_sigma = sigma_low;
    int blur_passes = 1;

    nux::ObjectPtr<nux::IOpenGLBaseTexture> device_texture = gpu_device->backup_texture0_;
    nux::ObjectPtr<nux::CachedBaseTexture> noise_device_texture = graphics_engine->CacheResource(noise_texture_.GetPointer());

    unsigned int offset = 0;
    int quad_width = larger_blur_geometry.width;
    int quad_height = larger_blur_geometry.height;

    int down_size_factor = 4;
    unsigned int buffer_width = quad_width + 2 * offset;
    unsigned int buffer_height = quad_height + 2 * offset;

    int x = (buffer_width - quad_width) / 2;
    int y = (buffer_height - quad_height) / 2;

    unsigned int down_size_width = buffer_width / down_size_factor;
    unsigned int down_size_height = buffer_height / down_size_factor;

    nux::TexCoordXForm texxform;
    nux::TexCoordXForm noise_texxform;

    texxform.SetFilter(nux::TEXFILTER_NEAREST, nux::TEXFILTER_NEAREST);

    noise_texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    noise_texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
    noise_texxform.SetFilter(nux::TEXFILTER_NEAREST, nux::TEXFILTER_NEAREST);

    // Copy source texture
    nux::ObjectPtr<nux::IOpenGLBaseTexture> texture_copy;
    graphics_engine->QRP_GetCopyTexture(buffer_width, buffer_height,
                                        texture_copy, device_texture,
                                        texxform__bg, nux::color::White);

    // Down size
    nux::ObjectPtr<nux::IOpenGLBaseTexture> resized_texture;
    graphics_engine->QRP_GetCopyTexture(down_size_width, down_size_height,
                                        resized_texture, texture_copy,
                                        texxform, nux::color::White);

    // Blur at a lower resolution (less pixels to process)
    nux::ObjectPtr<nux::IOpenGLBaseTexture> low_res_blur;
    low_res_blur = graphics_engine->QRP_GetBlurTexture(x, y, down_size_width, down_size_height,
                                                       resized_texture, texxform,
                                                       nux::color::White,
                                                       gaussian_sigma, blur_passes);

    // Up size
    texxform.SetFilter(nux::TEXFILTER_LINEAR, nux::TEXFILTER_LINEAR);
    graphics_engine->QRP_GetCopyTexture(buffer_width, buffer_height, resized_texture,
                                        low_res_blur, texxform, nux::color::White);

    // Returns a smaller blur region (minus blur radius).
    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    texxform.flip_v_coord = true;
    texxform.uoffset = dleft / (float) buffer_width;
    texxform.voffset = dbottom / (float) buffer_height;
    graphics_engine->QRP_GetCopyTexture(blur_geometry_.width, blur_geometry_.height,
                                        blur_texture_, resized_texture,
                                        texxform, nux::color::White);
  }

  if (current_fbo.IsValid())
  {
    current_fbo->Activate(true);
    graphics_engine->Push2DWindow(current_fbo->GetWidth(), current_fbo->GetHeight());
    graphics_engine->GetRenderStates ().EnableScissor (true);
  }
  else
  {
    graphics_engine->SetViewport(0, 0, monitor_width, monitor_height);
    graphics_engine->Push2DWindow(monitor_width, monitor_height);

    graphics_engine->ApplyClippingRectangle();
  }

  cache_dirty = false;
  return blur_texture_;
}

nux::ObjectPtr<nux::IOpenGLBaseTexture> BackgroundEffectHelper::GetRegion(bool force_update)
{
  bool should_update = force_update || cache_dirty;

  /* Static blur: only update when the size changed */
  if ((!should_update)
      && blur_texture_.IsValid()
      && (requested_blur_geometry_ == blur_geometry_))
  {
    return blur_texture_;
  }

  nux::GraphicsEngine* graphics_engine = nux::GetGraphicsDisplay()->GetGraphicsEngine();

  int monitor_width = BackgroundEffectHelper::monitor_rect_.width;
  int monitor_height = BackgroundEffectHelper::monitor_rect_.height;

  nux::Geometry temp = requested_blur_geometry_;
  temp.OffsetPosition(-monitor_rect_.x, -monitor_rect_.y);

  blur_geometry_ =  nux::Geometry(0, 0, monitor_width, monitor_height).Intersect(temp);

  nux::GpuDevice* gpu_device = nux::GetGraphicsDisplay()->GetGpuDevice();

  if (blur_geometry_.IsNull() || !gpu_device->backup_texture0_.IsValid())
  {
    return nux::ObjectPtr<nux::IOpenGLBaseTexture>();
  }

  // save the current fbo
  nux::ObjectPtr<nux::IOpenGLFrameBufferObject> current_fbo = gpu_device->GetCurrentFrameBufferObject();
  gpu_device->DeactivateFrameBuffer();

  // Set a viewport to the requested size
  // FIXME: We need to do multiple passes for the dirty region
  // on the underlying backup texture so that we're only updating
  // the bits that we need
  graphics_engine->SetViewport(0, 0, blur_geometry_.width, blur_geometry_.height);
  graphics_engine->SetScissor(0, 0, blur_geometry_.width, blur_geometry_.height);
  // Disable nux scissoring
  graphics_engine->GetRenderStates().EnableScissor(false);

  // The background texture is the same size as the monitor where we are rendering.
  nux::TexCoordXForm texxform__bg;
  texxform__bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform__bg.uoffset = ((float) blur_geometry_.x) / monitor_width;
  texxform__bg.voffset = ((float) blur_geometry_.y) / monitor_height;

  {
    nux::ObjectPtr<nux::IOpenGLBaseTexture> device_texture = gpu_device->backup_texture0_;

    unsigned int offset = 0;
    int quad_width = blur_geometry_.width;
    int quad_height = blur_geometry_.height;

    unsigned int buffer_width = quad_width + 2 * offset;
    unsigned int buffer_height = quad_height + 2 * offset;

    texxform__bg.SetFilter(nux::TEXFILTER_NEAREST, nux::TEXFILTER_NEAREST);
    texxform__bg.flip_v_coord = true;

    // Copy source texture
    graphics_engine->QRP_GetCopyTexture(buffer_width, buffer_height,
                                        blur_texture_, device_texture,
                                        texxform__bg, nux::color::White);
  }

  if (current_fbo.IsValid())
  {
    current_fbo->Activate(true);
    graphics_engine->Push2DWindow(current_fbo->GetWidth(), current_fbo->GetHeight());
    graphics_engine->GetRenderStates().EnableScissor(true);
  }
  else
  {
    graphics_engine->SetViewport(0, 0, monitor_width, monitor_height);
    graphics_engine->Push2DWindow(monitor_width, monitor_height);

    graphics_engine->ApplyClippingRectangle();
  }

  cache_dirty = false;
  return blur_texture_;
}

