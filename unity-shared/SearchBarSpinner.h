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

#ifndef SEARCH_BAR_SPINNER_H
#define SEARCH_BAR_SPINNER_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/TextureArea.h>
#include <NuxCore/Math/Matrix4.h>
#include <UnityCore/GLibSource.h>
#include "unity-shared/Introspectable.h"

namespace unity
{

enum SpinnerState
{
  STATE_READY,
  STATE_SEARCHING,
  STATE_CLEAR
};

class SearchBarSpinner : public unity::debug::Introspectable, public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(SearchBarSpinner, nux::View);
public:
  SearchBarSpinner();

  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

  void SetState(SpinnerState state);
  SpinnerState GetState() const;

  void SetSpinnerTimeout(int timeout);

protected:
  // Introspectable methods
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

  // Key navigation
  virtual bool AcceptKeyNavFocus();

private:
  bool OnFrameTimeout();

  SpinnerState state_;

  nux::BaseTexture* magnify_;
  nux::BaseTexture* circle_;
  nux::BaseTexture* close_;
  nux::BaseTexture* spin_;

  glib::Source::UniquePtr spinner_timeout_;
  glib::Source::UniquePtr frame_timeout_;
  int search_timeout_;

  nux::Matrix4 rotate_;
  float rotation_;
};

}

#endif
