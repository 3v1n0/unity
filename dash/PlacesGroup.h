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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 */

#ifndef UNITYSHELL_PLACES_GROUP_H
#define UNITYSHELL_PLACES_GROUP_H

#include <Nux/Nux.h>
#include <Nux/VLayout.h>
#include <Nux/HLayout.h>
#include <Nux/TextureArea.h>

#include <sigc++/sigc++.h>

#include "unity-shared/IconTexture.h"
#include "unity-shared/Introspectable.h"
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/UBusWrapper.h"

#include <UnityCore/GLibSource.h>

#include "AbstractPlacesGroup.h"

#include "ResultView.h"

namespace nux
{
class AbstractPaintLayer;
}

namespace unity
{


class HSeparator;

class PlacesGroup : public dash::AbstractPlacesGroup, public debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(PlacesGroup, dash::AbstractPlacesGroup);
public:

  PlacesGroup();

  void SetIcon(std::string const& icon);
  void SetName(std::string const& name);
  void SetRendererName(const char *renderer_name);
  void SetHeaderCountVisible(bool disable);

  nux::StaticCairoText* GetLabel();
  nux::StaticCairoText* GetExpandLabel();

  void SetChildView(dash::ResultView* view);
  nux::View* GetChildView();

  void SetChildLayout(nux::Layout* layout);

  void Relayout();

  void SetCategoryIndex(unsigned index);
  unsigned GetCategoryIndex() const;

  void SetCounts(unsigned n_visible_items_in_unexpand_mode,
                 unsigned n_total_items);

  void SetExpanded(bool is_expanded);
  bool GetExpanded() const;

  int  GetHeaderHeight() const;
  bool HeaderIsFocusable() const;
  nux::View* GetHeaderFocusableView() const;

  sigc::signal<void, PlacesGroup*> expanded;
  sigc::signal<void, std::string const&> UriActivated;

protected:
  long ComputeContentSize();
  void Draw(nux::GraphicsEngine& graphics_engine, bool force_draw);
  void DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw);
  void PostDraw (nux::GraphicsEngine &graphics_engine, bool force_draw);

  // Key navigation
  virtual bool AcceptKeyNavFocus();

  // Introspection
  virtual std::string GetName() const;
  virtual void AddProperties(GVariantBuilder* builder);

private:
  void Refresh();

  bool HeaderHasKeyFocus() const;
  bool ShouldBeHighlighted() const;

  void DrawSeparatorChanged(bool draw);
  void RecvMouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void OnLabelActivated(nux::Area* label);
  void OnLabelFocusChanged(nux::Area* label, bool has_focus, nux::KeyNavDirection direction);
  bool OnIdleRelayout();
  void RefreshLabel();

private:
  nux::VLayout* _group_layout;
  nux::View* _header_view;
  nux::HLayout* _header_layout;
  nux::HLayout* _text_layout;
  nux::HLayout* _expand_label_layout;
  nux::HLayout* _expand_layout;
  nux::View*  _child_view;
  std::unique_ptr<nux::AbstractPaintLayer> _focus_layer;

  IconTexture*          _icon;
  nux::StaticCairoText* _name;
  nux::StaticCairoText* _expand_label;
  IconTexture*          _expand_icon;

  nux::BaseTexture* _background;
  nux::BaseTexture* _background_nofilters;
  bool              _using_nofilters_background;
  std::unique_ptr<nux::AbstractPaintLayer> _background_layer;

  bool  _is_expanded;
  unsigned _n_visible_items_in_unexpand_mode;
  unsigned _n_total_items;
  unsigned _category_index;
  std::string _cached_name;
  nux::Geometry _cached_geometry;
  
  std::string _renderer_name;
  bool _coverflow_enabled;

  bool disabled_header_count_;

  glib::Source::UniquePtr _relayout_idle;
  UBusManager _ubus;
};

}

#endif
