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

#ifndef UNITY_DECORATED_WINDOW
#define UNITY_DECORATED_WINDOW

#include "Introspectable.h"
#include <NuxCore/Property.h>
#include <memory>

class CompRegion;
class CompWindow;
class GLWindowPaintAttrib;
class GLMatrix;
namespace compiz { namespace window { namespace extents { class Extents; } } }

namespace unity
{
namespace decoration
{
class Window : public debug::Introspectable
{
public:
  typedef std::shared_ptr<Window> Ptr;

  Window(CompWindow*);
  virtual ~Window() = default;

  nux::RWProperty<std::string> title;
  nux::Property<bool> scaled;
  nux::ROProperty<double> dpi_scale;

  CompWindow* GetCompWindow();
  void Update();
  void Undecorate();
  void UpdateDecorationPosition();
  void UpdateDecorationPositionDelayed();
  void UpdateFrameRegion(CompRegion&);
  void UpdateOutputExtents(compiz::window::extents::Extents&);
  void Paint(GLMatrix const&, GLWindowPaintAttrib const&, CompRegion const&, unsigned mask);
  void Draw(GLMatrix const&, GLWindowPaintAttrib const&, CompRegion const&, unsigned mask);

protected:
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);
  IntrospectableList GetIntrospectableChildren();

private:
  Window(Window const&) = delete;
  Window& operator=(Window const&) = delete;

  friend class Manager;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // decoration namespace
} // unity namespace

#endif
