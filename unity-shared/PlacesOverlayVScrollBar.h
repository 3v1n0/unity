// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012-2015 Canonical Ltd
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef PLACES_OVERLAY_VSCROLLBAR_H
#define PLACES_OVERLAY_VSCROLLBAR_H

#include <Nux/Nux.h>
#include <Nux/InputAreaProximity.h>
#include <NuxCore/Animation.h>
#include <UnityCore/ConnectionManager.h>
#include <memory>

#include "unity-shared/PlacesVScrollBar.h"

namespace unity
{
namespace dash
{

enum class ScrollDir : unsigned int
{
  UP,
  DOWN,
};

class PlacesOverlayVScrollBar: public PlacesVScrollBar
{
public:
  PlacesOverlayVScrollBar(NUX_FILE_LINE_PROTO);
  virtual ~PlacesOverlayVScrollBar() {}

  void PerformPageNavigation(ScrollDir);

private:
  void UpdateScrollbarSize();
  void StartScrollAnimation(ScrollDir, int stop, unsigned duration);

  class ProximityArea;
  std::shared_ptr<ProximityArea> area_prox_;

  nux::animation::AnimateValue<int> animation_;
  connection::Wrapper tweening_connection_;
  int delta_update_;

  friend class MockScrollBar;
};

} // namespace dash
} // namespace unity

#endif // PLACES_OVERLAY_VSCROLLBAR_H
