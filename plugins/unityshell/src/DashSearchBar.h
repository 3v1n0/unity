// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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

#ifndef DASH_SEARCH_BAR_H
#define DASH_SEARCH_BAR_H

#include <Nux/LayeredLayout.h>
#include <Nux/TextureArea.h>
#include <Nux/View.h>
#include <Nux/TextureArea.h>
#include <NuxGraphics/GraphicsEngine.h>

#include "Introspectable.h"

#include "Nux/EditTextBox.h"
#include "Nux/TextEntry.h"

#include "DashSearchBarSpinner.h"
#include <gtk/gtk.h>

#include "StaticCairoText.h"

namespace unity
{
namespace dash
{

class SearchBar : public unity::Introspectable, public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(SearchBar, nux::View);
public:
  SearchBar(NUX_FILE_LINE_PROTO);
  ~SearchBar();

  virtual long ProcessEvent(nux::IEvent& ievent, long TraverseInfo, long ProcessEventInfo);
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

  /*
  void SetActiveEntry(PlaceEntry* entry,
                      guint       section_id,
                      const char* search_string);
                      */
  void OnSearchFinished();

  sigc::signal<void, const char*> search_changed;
  sigc::signal<void> activated;

  nux::TextEntry* GetTextEntry()
  {
    return _pango_entry;
  }

  void RecvMouseDownFromWindow(int x, int y, unsigned long button_flags, unsigned long key_flags);

private:
  // Introspectable methods
  const gchar* GetName();
  const gchar* GetChildsName();
  void AddProperties(GVariantBuilder* builder);

  // Key navigation
  bool AcceptKeyNavFocus();

  void UpdateBackground();
  void OnSearchChanged(nux::TextEntry* text_entry);
  void EmitLiveSearch();
  void OnClearClicked(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void OnEntryActivated();
  void OnLayeredLayoutQueueDraw(int i);

  static bool OnLiveSearchTimeout(SearchBar* self);
  static void OnFontChanged(GObject* object, GParamSpec* pspec, SearchBar* self);
  static void OnDashClosed(GVariant* variant, SearchBar* self);

private:
  nux::AbstractPaintLayer* _bg_layer;
  nux::HLayout*            _layout;
  nux::LayeredLayout*      _layered_layout;
  nux::StaticCairoText*    _hint;
  nux::TextEntry*          _pango_entry;
  int _last_width;
  int _last_height;
  guint                    _live_search_timeout;
  guint32                  _font_changed_id;

  SearchBarSpinner*  _spinner;
};

}
}

#endif
