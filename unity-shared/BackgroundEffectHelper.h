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

#ifndef BACKGROUND_EFFECT_HELPER_H
#define BACKGROUND_EFFECT_HELPER_H

#include "config.h"

#include <Nux/Nux.h>
#include <NuxGraphics/GLThread.h>
#include <UnityCore/ConnectionManager.h>

namespace unity
{

enum BlurType
{
  BLUR_NONE,
  BLUR_STATIC,
  BLUR_ACTIVE
};

class BackgroundEffectHelper
{
public:
  BackgroundEffectHelper();
  BackgroundEffectHelper(nux::View*);
  ~BackgroundEffectHelper();

  nux::Property<nux::View*> owner;
  nux::Property<bool> enabled;

  nux::ObjectPtr<nux::IOpenGLBaseTexture> GetBlurRegion(bool force_update = false);
  nux::ObjectPtr<nux::IOpenGLBaseTexture> GetRegion(bool force_update = false);
  // We could add more functions here to get different types of effects based on the background texture
  // nux::ObjectPtr<nux::IOpenGLBaseTexture> GetPixelatedRegion(nux::Rect rect, int pixel_size, bool update);

  typedef std::function<nux::Geometry()> GeometryGetterFunc;
  void SetGeometryGetter(GeometryGetterFunc const&);

  void DirtyCache();

  static void ProcessDamage(nux::Geometry const& geo);
  static bool HasDirtyHelpers();
  static bool HasEnabledHelpers();
  static std::vector<nux::Geometry> const& GetBlurGeometries();

  static nux::Property<unity::BlurType> blur_type;
  static nux::Geometry monitor_rect_;

  static sigc::signal<void, nux::Geometry const&> blur_region_needs_update_;

protected:
  static void Register   (BackgroundEffectHelper* self);
  static void Unregister (BackgroundEffectHelper* self);

private:
  static float GetBlurSigma();
  static int GetBlurRadius();
  static void UpdateBlurGeometries();

  void OnEnabledChanged(bool value);
  void OnOwnerChanged(nux::View*);
  void SetupOwner(nux::View*);
  bool UpdateOwnerGeometry();

  nux::ObjectPtr<nux::BaseTexture> noise_texture_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> blur_texture_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> resize_tmp_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> noisy_tmp_;
  nux::FxStructure blur_fx_struct_;
  nux::FxStructure noise_fx_struct_;
  nux::Geometry blur_geometry_;
  nux::Geometry requested_blur_geometry_;
  GeometryGetterFunc geo_getter_func_;
  connection::Manager connections_;

  bool cache_dirty;

  static std::list<BackgroundEffectHelper*> registered_list_;
  static std::vector<nux::Geometry> blur_geometries_;
};

}

#endif

