// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef PLACES_SEARCH_BAR_SPINNER_H
#define PLACES_SEARCH_BAR_SPINNER_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/TextureArea.h>
#include <NuxCore/Math/Matrix4.h>
#include "Introspectable.h"

enum SpinnerState
{
  STATE_READY,
  STATE_SEARCHING,
  STATE_CLEAR
};

class PlacesSearchBarSpinner : public Introspectable, public nux::View
{
  NUX_DECLARE_OBJECT_TYPE (PlacesSearchBarSpinner, nux::View);
public:
  PlacesSearchBarSpinner ();
  ~PlacesSearchBarSpinner ();

  long ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
  void Draw (nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw);

  void SetState (SpinnerState state);

protected:
  // Introspectable methods
  const gchar * GetName ();
  void AddProperties (GVariantBuilder *builder);
  static gboolean OnFrame (PlacesSearchBarSpinner *self);

private:

private:
  SpinnerState      _state;

  nux::BaseTexture *_magnify;
  nux::BaseTexture *_close;
  nux::BaseTexture *_close_glow;
  nux::BaseTexture *_spin;
  nux::BaseTexture *_spin_glow;

  nux::Matrix4 _2d_rotate;
  float        _rotation;

  guint32 _spinner_timeout;
};

#endif
