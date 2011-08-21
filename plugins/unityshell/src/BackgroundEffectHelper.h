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

#include "Nux/Nux.h"
#include "Nux/ColorArea.h"
#include "NuxGraphics/GLThread.h"

namespace unity
{

enum BlurType
{
  BLUR_NONE,
  BLUR_STATIC,
  BLUR_ACTIVE
};

}

class BackgroundEffectHelper
{
public:
  BackgroundEffectHelper();
  ~BackgroundEffectHelper();

  nux::Property<nux::View*> owner;
  nux::Property<bool> enabled;

  nux::ObjectPtr<nux::IOpenGLBaseTexture> GetBlurRegion(nux::Geometry geo, bool force_update = false);
  // We could add more functions here to get different types of effects based on the background texture
  nux::ObjectPtr<nux::IOpenGLBaseTexture> GetPixelatedRegion(nux::Rect rect, int pixel_size, bool update);

  void DirtyCache();

  static void QueueDrawOnOwners();

  static bool HasEnabledHelpers();
  static void SetDamageBounds(Region damage);
  static void ResetDamageBounds();
  static void AddOccludedRegion(Region occluded);
  static void ResetOcclusionBuffer();
  static bool OcclusionDetectionActive();

  static nux::Property<unity::BlurType> blur_type;
  static nux::Property<float> sigma_high;
  static nux::Property<float> sigma_med;
  static nux::Property<float> sigma_low;
  static nux::Property<bool> updates_enabled;
  static nux::Property<bool> detecting_occlusions;

protected:
  static void Register   (BackgroundEffectHelper* self);
  static void Unregister (BackgroundEffectHelper* self);

private:
  void OnEnabledChanged (bool value);

  nux::BaseTexture*                       noise_texture_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> temp_device_texture0_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> temp_device_texture1_;

  nux::ObjectPtr<nux::IOpenGLBaseTexture> ds_temp_device_texture0_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> ds_temp_device_texture1_;

  nux::ObjectPtr<nux::IOpenGLBaseTexture> blur_texture_;
  nux::Geometry blur_geometry_;

  static std::list<BackgroundEffectHelper*> registered_list_;

  // FIXME: I couldn't find anything in nux that works
  // like X Regions do
  static Region occluded_region_;
  static Region damage_region_;
};

#endif

