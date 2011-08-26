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

#ifndef PLACES_GROUP_H
#define PLACES_GROUP_H

#include <Nux/Nux.h>
#include <Nux/VLayout.h>
#include <Nux/HLayout.h>
#include <Nux/TextureArea.h>

#include <sigc++/sigc++.h>

#include "IconTexture.h"
#include "Introspectable.h"
#include "StaticCairoText.h"

class PlacesGroup : public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(PlacesGroup, nux::View);
public:

  PlacesGroup();
  ~PlacesGroup();

  void SetIcon(const char* icon);
  void SetName(const char* name);

  const char* GetName();

  void       SetChildView(nux::View* view);
  nux::View* GetChildView();

  void SetChildLayout(nux::Layout* layout);

  void Relayout();

  void SetCounts(guint n_visible_items_in_unexpand_mode, guint n_total_items);

  void SetExpanded(bool is_expanded);
  bool GetExpanded();

  int  GetHeaderHeight();

  void SetDrawSeparator(bool draw_it);

  sigc::signal<void, PlacesGroup*> expanded;

protected:
  // Key navigation
  virtual bool AcceptKeyNavFocus();

private:
  void Refresh();

  long ProcessEvent(nux::IEvent& ievent, long TraverseInfo, long ProcessEventInfo);
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

  static gboolean OnIdleRelayout(PlacesGroup* self);

  void RecvMouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void OnLabelActivated(nux::Area* label);
  void OnLabelFocusChanged(nux::Area* label);
  void RefreshLabel();

private:
  nux::VLayout* _group_layout;
  nux::HLayout* _header_layout;
  nux::View*  _child_view;

  IconTexture*          _icon;
  nux::StaticCairoText* _name;
  nux::StaticCairoText* _expand_label;
  IconTexture*          _expand_icon;

  guint32 _idle_id;

  bool  _is_expanded;
  guint _n_visible_items_in_unexpand_mode;
  guint _n_total_items;
  char* _cached_name;
  bool  _draw_sep;

};

#endif
