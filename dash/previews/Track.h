// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012 Canonical Ltd.
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#ifndef TRACK_H
#define TRACK_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <UnityCore/Tracks.h>
#include "unity-shared/Introspectable.h"

namespace nux
{
  class StaticCairoText;
  class LayeredLayout;
}

namespace unity
{
class IconTexture;

namespace dash
{
namespace previews
{

class Track : public nux::View, public debug::Introspectable
{
public:
  typedef nux::ObjectPtr<Track> Ptr;
  NUX_DECLARE_OBJECT_TYPE(Track, nux::View);

  Track(NUX_FILE_LINE_PROTO);
  virtual ~Track();

  void Update(dash::Track const& track_row);
 
  // From debug::Introspectable
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  sigc::signal<void, std::string const&> play;
  sigc::signal<void, std::string const&> pause;

protected:
  virtual void Draw(nux::GraphicsEngine& gfx_engine, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw);
  virtual long ComputeContentSize();
  
  void SetupBackground();
  void SetupViews();

  bool HasStatusFocus() const;

  void OnTrackControlMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void OnTrackControlMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags);

  void UpdateTrackState();

protected:
  std::string uri_;
  float progress_;
  nux::StaticCairoText* track_number_;
  nux::StaticCairoText* title_;
  nux::StaticCairoText* duration_;

  typedef std::unique_ptr<nux::AbstractPaintLayer> LayerPtr;
  LayerPtr focus_layer_;
  LayerPtr progress_layer_;

  nux::Layout* title_layout_;
  nux::Layout* duration_layout_;
  nux::View* status_play_layout_;
  nux::View* status_pause_layout_;
  nux::View* track_number_layout_;
  nux::LayeredLayout* track_status_layout_;

  dash::PlayState play_state_;
  bool mouse_over_;
};

}
}
}

#endif // TRACK_H
