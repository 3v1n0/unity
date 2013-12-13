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

#ifndef UNITY_DECORATION_WIDGETS
#define UNITY_DECORATION_WIDGETS

#include <list>
#include "CompizUtils.h"

namespace unity
{
namespace decoration
{
namespace cu = compiz_utils;

class Item : public sigc::trackable
{
public:
  typedef std::shared_ptr<Item> Ptr;

  Item();
  virtual ~Item() = default;

  nux::Property<bool> visible;

  virtual CompRect const& Geometry() const = 0;
  virtual int GetNaturalWidth() const;
  virtual int GetNaturalHeight() const;

  virtual void SetCoords(int x, int y);
  virtual void SetX(int x) { SetCoords(x, Geometry().y()); }
  virtual void SetY(int y) { SetCoords(Geometry().x(), y); }
  virtual void SetSize(int width, int height);
  virtual void SetWidth(int width) { SetSize(width, Geometry().height()); }
  virtual void SetHeight(int height)  { SetSize(Geometry().width(), height); };

  virtual void SetMaxWidth(int max_width);
  virtual void SetMaxHeight(int max_height);
  virtual void SetMinWidth(int min_width);
  virtual void SetMinHeight(int min_height);

  int GetMaxWidth() const { return max_.width; };
  int GetMaxHeight() const { return max_.height; };
  int GetMinWidth() const { return min_.width; };
  int GetMinHeight() const { return min_.height; };

  void Damage();
  virtual void Draw(GLWindow*, GLMatrix const&, GLWindowPaintAttrib const&, CompRegion const&, unsigned mask) {}

protected:
  CompRect& InternalGeo();
  sigc::signal<void> geo_parameters_changed;

  nux::Size max_;
  nux::Size min_;
  nux::Size natural_;
};


class TexturedItem : public Item
{
public:
  typedef std::shared_ptr<TexturedItem> Ptr;

  TexturedItem();

  void Draw(GLWindow*, GLMatrix const&, GLWindowPaintAttrib const&, CompRegion const&, unsigned mask);
  void SetCoords(int x, int y);

  CompRect const& Geometry() const;
  int GetNaturalWidth() const;
  int GetNaturalHeight() const;

protected:
  cu::SimpleTextureQuad texture_;
};

class SimpleItem : public Item
{
public:
  CompRect const& Geometry() const { return rect_; }

protected:
  CompRect rect_;
};

class Layout : public SimpleItem
{
public:
  typedef std::shared_ptr<Layout> Ptr;

  Layout();

  void Append(Item::Ptr const&);

  CompRect const& Geometry() const;
  void Relayout();

  void Draw(GLWindow*, GLMatrix const&, GLWindowPaintAttrib const&, CompRegion const&, unsigned mask);

  std::list<Item::Ptr> const& Items() const;

private:
  int inner_padding_;
  std::list<Item::Ptr> items_;
};

} // decoration namespace
} // unity namespace

#endif
