// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef UNITY_DECORATIONS_DATA_POOL
#define UNITY_DECORATIONS_DATA_POOL

#include <unordered_map>
#include "DecorationStyle.h"
#include "DecorationsEdge.h"

namespace unity
{
namespace decoration
{

class DataPool : public sigc::trackable
{
public:
  typedef std::shared_ptr<DataPool> Ptr;

  static DataPool::Ptr const& Get();
  static void Reset();
  virtual ~DataPool() = default;

  Cursor EdgeCursor(Edge::Type) const;
  cu::SimpleTexture::Ptr const& GlowTexture() const;
  cu::SimpleTexture::Ptr const& ButtonTexture(WindowButtonType, WidgetState) const;
  cu::SimpleTexture::Ptr const& ButtonTexture(double scale, WindowButtonType, WidgetState) const;

private:
  DataPool();
  DataPool(DataPool const&) = delete;
  DataPool& operator=(DataPool const&) = delete;

  void SetupTextures();

  cu::SimpleTexture::Ptr glow_texture_;

  typedef std::array<std::array<cu::SimpleTexture::Ptr, size_t(WidgetState::Size)>, size_t(WindowButtonType::Size)> WindowButtonsArray;
  WindowButtonsArray window_buttons_;
  std::unordered_map<double, WindowButtonsArray> scaled_window_buttons_;
};

} // decoration namespace
} // unity namespace

#endif // UNITY_DECORATIONS_DATA_POOL
