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

#ifndef SEARCH_BAR_H
#define SEARCH_BAR_H

#include <gtk/gtk.h>
#include <NuxCore/Property.h>
#include <Nux/LayeredLayout.h>
#include <Nux/VLayout.h>
#include <Nux/TextEntry.h>
#include <UnityCore/GLibSignal.h>
#include <UnityCore/GLibSource.h>

#include "SearchBarSpinner.h"
#include "unity-shared/IconTexture.h"
#include "unity-shared/IMTextEntry.h"
#include "unity-shared/Introspectable.h"
#include "unity-shared/StaticCairoText.h"

namespace nux
{
class AbstractPaintLayer;
class LinearLayout;
}

namespace unity
{

class SearchBar : public unity::debug::Introspectable, public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(SearchBar, nux::View);
public:
  typedef nux::ObjectPtr<SearchBar> Ptr;
  SearchBar(NUX_FILE_LINE_PROTO);
  SearchBar(bool show_filter_hint, NUX_FILE_LINE_PROTO);

  void ForceLiveSearch();
  void SetSearchFinished();

  nux::TextEntry* text_entry() const;
  nux::View* show_filters() const;

  nux::RWProperty<std::string> search_string;
  nux::Property<std::string> search_hint;
  nux::Property<bool> showing_filters;
  nux::Property<bool> can_refine_search;
  nux::ROProperty<bool> im_active;
  nux::ROProperty<bool> im_preedit;

  sigc::signal<void> activated;
  sigc::signal<void, std::string const&> search_changed;
  sigc::signal<void, std::string const&> live_search_reached;

private:
  void Init();

  void OnFontChanged(GtkSettings* settings, GParamSpec* pspec=NULL);
  void OnSearchHintChanged();

  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

  void OnMouseButtonDown(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void OnEndKeyFocus();

  void UpdateBackground(bool force);
  void OnSearchChanged(nux::TextEntry* text_entry);
  void OnClearClicked(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void OnEntryActivated();
  void OnShowingFiltersChanged(bool is_showing);
  bool OnLiveSearchTimeout();
  bool OnSpinnerStartCb();

  std::string get_search_string() const;
  bool set_search_string(std::string const& string);
  bool get_im_active() const;
  bool get_im_preedit() const;
  bool show_filter_hint_;

  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);
  bool AcceptKeyNavFocus();

private:
  bool ShouldBeHighlighted();
  
  std::unique_ptr<nux::AbstractPaintLayer> bg_layer_;
  std::unique_ptr<nux::AbstractPaintLayer> highlight_layer_;
  nux::HLayout* layout_;
  nux::HLayout* entry_layout_;
  nux::LayeredLayout* layered_layout_;
  StaticCairoText* hint_;
  nux::LinearLayout* expander_layout_;
  IMTextEntry* pango_entry_;
  nux::View* expander_view_;
  nux::HLayout* filter_layout_;
  StaticCairoText* show_filters_;
  nux::VLayout* arrow_layout_;
  nux::SpaceLayout* arrow_top_space_;
  nux::SpaceLayout* arrow_bottom_space_;
  IconTexture* expand_icon_;

  int last_width_;
  int last_height_;

  glib::SignalManager sig_manager_;
  glib::Source::UniquePtr live_search_timeout_;
  glib::Source::UniquePtr start_spinner_timeout_;

  SearchBarSpinner* spinner_;
};

}

#endif
