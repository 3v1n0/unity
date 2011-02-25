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

#include <sigc++/sigc++.h>

#include <Nux/Nux.h>
#include <Nux/VLayout.h>
#include <Nux/BaseWindow.h>
#include <NuxCore/Math/MathInc.h>

#include "StaticCairoText.h"

#include "Introspectable.h"

#include <sigc++/trackable.h>
#include <sigc++/signal.h>
#include <sigc++/functors/ptr_fun.h>
#include <sigc++/functors/mem_fun.h>

#include "IconTexture.h"

class PlacesGroup : public nux::View
{
public:

  PlacesGroup (NUX_FILE_LINE_PROTO);
  ~PlacesGroup ();

  void SetTitle (const char *title);
  void SetEmblem (const char *path_to_emblem);

  void AddLayout (nux::Layout *layout);
  nux::Layout *GetLayout ();
  void SetRowHeight (unsigned int row_height);
  void SetItemDetail (unsigned int total_items, unsigned int visible_items);
  void SetExpanded (bool expanded);
  void Relayout ();

protected:
  nux::StaticCairoText *_label;
  nux::StaticCairoText *_title;

  char *_title_string;
  unsigned int _row_height;
  unsigned int _total_items;
  unsigned int _visible_items;

  nux::Layout *_content;
  IconTexture  *_icon_texture;
  nux::VLayout *_group_layout;
  nux::HLayout *_header_layout;

  bool _expanded;

  guint32 _idle_id;

  static gboolean OnIdleRelayout (PlacesGroup *self);
  void UpdateTitle ();
  void UpdateLabel ();

  virtual long ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
  virtual void Draw (nux::GraphicsEngine &GfxContext, bool force_draw);
  virtual void DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw);
  virtual void PostDraw (nux::GraphicsEngine &GfxContext, bool force_draw);

  virtual void PreLayoutManagement ();
  virtual long PostLayoutManagement (long LayoutResult);

  void RecvMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseClick (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseMove (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);

  sigc::signal<void, PlacesGroup*> sigMouseEnter;
  sigc::signal<void, PlacesGroup*> sigMouseLeave;
  sigc::signal<void, PlacesGroup*, int, int> sigMouseReleased;
  sigc::signal<void, PlacesGroup*, int, int> sigMouseClick;
  sigc::signal<void, PlacesGroup*, int, int> sigMouseDrag;
};

#endif
