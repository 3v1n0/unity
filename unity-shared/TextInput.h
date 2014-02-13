// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Manuel de la Pena <manuel.delapena@canonical.com>
 */

#ifndef TEXTINPUT_H
#define TEXTINPUT_H

#include "config.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/LayeredLayout.h>
#include <Nux/VLayout.h>
#include <Nux/TextEntry.h>
#include <NuxCore/Logger.h>
#include <NuxCore/Property.h>
#include <UnityCore/GLibSignal.h>
#include <UnityCore/GLibSource.h>

#include "CairoTexture.h"
#include "unity-shared/IconTexture.h"
#include "unity-shared/IMTextEntry.h"
#include "unity-shared/Introspectable.h"
#include "unity-shared/SearchBarSpinner.h"
#include "unity-shared/StaticCairoText.h"

namespace nux
{
class AbstractPaintLayer;
class LinearLayout;
}

namespace unity
{

class TextInput : public unity::debug::Introspectable, public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(TextInput, nux::View);

public:
  typedef nux::ObjectPtr<TextInput> Ptr;
  TextInput(NUX_FILE_LINE_PROTO);
  TextInput(bool show_filter_hint, NUX_FILE_LINE_PROTO);

  void SetSpinnerVisible(bool visible);
  void SetSpinnerState(SpinnerState spinner_state);

  nux::TextEntry* text_entry() const;

  nux::RWProperty<std::string> input_string;
  nux::Property<std::string> input_hint;
  nux::ROProperty<bool> im_active;
  nux::ROProperty<bool> im_preedit;

private:

  void OnFontChanged(GtkSettings* settings, GParamSpec* pspec=NULL);
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  void UpdateBackground(bool force);

  std::string GetName() const;

  void AddProperties(debug::IntrospectionData&);
  bool AcceptKeyNavFocus();

protected:

  void Init();
  void OnInputHintChanged();
  void OnMouseButtonDown(int x, int y, unsigned long button_flags,
          unsigned long key_flags);
  void OnEndKeyFocus();

  // getters & setters

  std::string get_input_string() const;
  bool set_input_string(std::string const& string);
  bool get_im_active() const;
  bool get_im_preedit() const;

  // instance vars
  StaticCairoText* hint_;
  IMTextEntry* pango_entry_;

private:

  bool ShouldBeHighlighted();

  std::unique_ptr<nux::AbstractPaintLayer> bg_layer_;
  std::unique_ptr<nux::AbstractPaintLayer> highlight_layer_;
  nux::HLayout* layout_;
  nux::LayeredLayout* layered_layout_;
  SearchBarSpinner* spinner_;

  int last_width_;
  int last_height_;

  glib::SignalManager sig_manager_;

};

}

#endif
