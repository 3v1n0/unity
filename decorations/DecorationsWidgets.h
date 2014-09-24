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

#include <deque>
#include <NuxCore/Size.h>
#include <NuxCore/Property.h>
#include <UnityCore/UWeakPtr.h>
#include "Introspectable.h"
#include "CompizUtils.h"
#include "RawPixel.h"

namespace unity
{
namespace decoration
{
namespace cu = compiz_utils;

class BasicContainer;

class Item : public sigc::trackable, public debug::Introspectable
{
public:
  typedef std::shared_ptr<Item> Ptr;
  typedef unity::uweak_ptr<Item> WeakPtr;
  typedef std::deque<Item::Ptr> List;

  Item();
  virtual ~Item() = default;

  nux::Property<bool> visible;
  nux::Property<bool> focused;
  nux::Property<bool> sensitive;
  nux::Property<bool> mouse_owner;
  nux::Property<double> scale;

  CompRect const& Geometry() const;
  virtual int GetNaturalWidth() const;
  virtual int GetNaturalHeight() const;

  virtual void SetCoords(int x, int y);
  virtual void SetX(int x) { SetCoords(x, Geometry().y()); }
  virtual void SetY(int y) { SetCoords(Geometry().x(), y); }
  virtual void SetSize(int width, int height);
  virtual void SetWidth(int width) { SetSize(width, Geometry().height()); }
  virtual void SetHeight(int height) { SetSize(Geometry().width(), height); };

  virtual void SetMaxWidth(int max_width);
  virtual void SetMaxHeight(int max_height);
  virtual void SetMinWidth(int min_width);
  virtual void SetMinHeight(int min_height);

  int GetMaxWidth() const;
  int GetMaxHeight() const;
  int GetMinWidth() const;
  int GetMinHeight() const;

  void SetParent(std::shared_ptr<BasicContainer> const&);
  std::shared_ptr<BasicContainer> GetParent() const;
  std::shared_ptr<BasicContainer> GetTopParent() const;

  void Damage();
  virtual void Draw(GLWindow*, GLMatrix const&, GLWindowPaintAttrib const&, CompRegion const&, unsigned mask) {}

protected:
  virtual CompRect& InternalGeo() = 0;
  sigc::signal<void> geo_parameters_changed;

  virtual bool IsContainer() const { return false; }
  void RequestRelayout();

  friend class InputMixer;
  virtual void MotionEvent(CompPoint const&, Time) {}
  virtual void ButtonDownEvent(CompPoint const&, unsigned button, Time) {}
  virtual void ButtonUpEvent(CompPoint const&, unsigned button, Time) {}

  std::string GetName() const { return "Item"; }
  void AddProperties(debug::IntrospectionData&);

private:
  Item(Item const&) = delete;
  Item& operator=(Item const&) = delete;

protected:
  nux::Size max_;
  nux::Size min_;
  nux::Size natural_;

private:
  unity::uweak_ptr<BasicContainer> parent_;
};

class SimpleItem : public Item
{
protected:
  CompRect& InternalGeo() { return rect_; }
  CompRect rect_;
};


class TexturedItem : public Item
{
public:
  typedef std::shared_ptr<TexturedItem> Ptr;

  TexturedItem();

  void SetTexture(cu::SimpleTexture::Ptr const&);
  void Draw(GLWindow*, GLMatrix const&, GLWindowPaintAttrib const&, CompRegion const&, unsigned mask);
  void SetCoords(int x, int y);

  int GetNaturalWidth() const;
  int GetNaturalHeight() const;

protected:
  std::string GetName() const { return "TexturedItem"; }

  CompRect& InternalGeo();
  cu::SimpleTextureQuad texture_;

private:
  bool dirty_region_;
};


class BasicContainer : public SimpleItem, public std::enable_shared_from_this<BasicContainer>
{
public:
  typedef std::shared_ptr<BasicContainer> Ptr;

  BasicContainer();
  Item::List const& Items() const { return items_; }

  virtual CompRect ContentGeometry() const;

protected:
  friend class Item;
  void Relayout();
  bool IsContainer() const { return true; }

  std::string GetName() const { return "BasicContainer"; }
  void AddProperties(debug::IntrospectionData&);
  IntrospectableList GetIntrospectableChildren();

  Item::List items_;

private:
  virtual void DoRelayout() = 0;
  bool relayouting_;
};


class Layout : public BasicContainer
{
public:
  typedef std::shared_ptr<Layout> Ptr;

  Layout();

  nux::Property<RawPixel> inner_padding;
  nux::Property<RawPixel> left_padding;
  nux::Property<RawPixel> right_padding;
  nux::Property<RawPixel> top_padding;
  nux::Property<RawPixel> bottom_padding;

  void Append(Item::Ptr const&);
  void Remove(Item::Ptr const&);

  CompRect ContentGeometry() const;
  void Draw(GLWindow*, GLMatrix const&, GLWindowPaintAttrib const&, CompRegion const&, unsigned mask);

protected:
  std::string GetName() const { return "Layout"; }
  void AddProperties(debug::IntrospectionData&);
  void DoRelayout();

private:
  bool SetPadding(RawPixel& target, RawPixel const& new_value);
};

} // decoration namespace
} // unity namespace

#endif
