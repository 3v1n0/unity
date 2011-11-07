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

#include <gtk/gtk.h>

#include <NuxCore/Property.h>
#include <Nux/LayeredLayout.h>
#include <Nux/TextureArea.h>
#include <Nux/View.h>
#include <Nux/TextureArea.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/EditTextBox.h>
#include <Nux/TextEntry.h>
#include <UnityCore/GLibSignal.h>

#include "DashSearchBarSpinner.h"
#include "IMTextEntry.h"
#include "Introspectable.h"
#include "StaticCairoText.h"

namespace unity
{
namespace dash
{

using namespace unity::glib;

class SearchBar : public unity::Introspectable, public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(SearchBar, nux::View);
public:
  SearchBar(NUX_FILE_LINE_PROTO);
  ~SearchBar();

  void SearchFinished();
  nux::TextEntry* text_entry() const;

  nux::RWProperty<std::string> search_string;
  nux::Property<std::string> search_hint;
  nux::Property<bool> showing_filters;
  nux::Property<bool> can_refine_search;
  nux::ROProperty<bool> im_active;

  sigc::signal<void> activated;
  sigc::signal<void, std::string const&> search_changed;
  sigc::signal<void, std::string const&> live_search_reached;

private:

  void OnFontChanged(GtkSettings* settings, GParamSpec* pspec=NULL);
  void OnSearchHintChanged();

  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

  void OnMouseButtonDown(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void OnEndKeyFocus();

  void UpdateBackground();
  void OnSearchChanged(nux::TextEntry* text_entry);
  void OnClearClicked(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void OnEntryActivated();
  void OnShowingFiltersChanged(bool is_showing);

  std::string get_search_string() const;
  bool set_search_string(std::string const& string);
  bool get_im_active() const;

  static gboolean OnLiveSearchTimeout(SearchBar* self);

  std::string GetName() const;
  std::string GetChildsName();
  void AddProperties(GVariantBuilder* builder);
  bool AcceptKeyNavFocus();

private:
  glib::SignalManager sig_manager_;
  
  nux::AbstractPaintLayer* bg_layer_;
  nux::HLayout* layout_;
  nux::LayeredLayout* layered_layout_;
  nux::StaticCairoText* hint_;
  IMTextEntry* pango_entry_;
  nux::StaticCairoText* show_filters_;
  
  int last_width_;
  int last_height_;
  
  guint live_search_timeout_;

  SearchBarSpinner* spinner_;
};

}
}

#endif
