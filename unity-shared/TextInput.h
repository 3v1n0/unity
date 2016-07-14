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

#include <Nux/Nux.h>
#include <UnityCore/GLibSignal.h>
#include <UnityCore/GLibSource.h>

#include "Introspectable.h"
#include "IMTextEntry.h"
#include "RawPixel.h"
#include "SearchBarSpinner.h"

namespace nux
{
class AbstractPaintLayer;
class LayeredLayout;
class LinearLayout;
class HLayout;
}

namespace unity
{
class IconTexture;
class StaticCairoText;
class SearchBarSpinner;

class TextInput : public unity::debug::Introspectable, public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(TextInput, nux::View);

public:
  typedef nux::ObjectPtr<TextInput> Ptr;
  TextInput(NUX_FILE_LINE_PROTO);

  void SetSpinnerVisible(bool visible);
  void SetSpinnerState(SpinnerState spinner_state);

  void EnableCapsLockWarning() const;

  IMTextEntry* text_entry() const;

  nux::Property<std::string> activator_icon;
  nux::Property<RawPixel> activator_icon_size;
  nux::Property<nux::Color> background_color;
  nux::Property<nux::Color> border_color;
  nux::Property<int> border_radius;
  nux::RWProperty<std::string> input_string;
  nux::Property<std::string> input_hint;
  nux::Property<std::string> hint_font_name;
  nux::Property<int> hint_font_size;
  nux::Property<nux::Color> hint_color;
  nux::ROProperty<bool> im_active;
  nux::ROProperty<bool> im_preedit;
  nux::Property<bool> show_activator;
  nux::Property<bool> show_lock_warnings;
  nux::Property<double> scale;

private:
  void UpdateFont();
  void UpdateHintFont();
  void UpdateHintColor();
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  void UpdateBackground(bool force);
  void UpdateScale(double);
  void UpdateSize();
  void UpdateTextures();

  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

  bool AcceptKeyNavFocus();
  bool ShouldBeHighlighted();
  void CheckLocks();
  void OnLockStateChanged(bool);

  nux::ObjectPtr<nux::BaseTexture> LoadActivatorIcon(std::string const& icon_file, int icon_size);
  nux::ObjectPtr<nux::BaseTexture> LoadWarningIcon(int icon_size);
  void LoadWarningTooltip();

  void PaintWarningTooltip(nux::GraphicsEngine& graphics_engine);

protected:
  void OnInputHintChanged();
  void OnMouseButtonDown(int x, int y, unsigned long button_flags, unsigned long key_flags);
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
  std::unique_ptr<nux::AbstractPaintLayer> bg_layer_;
  std::unique_ptr<nux::AbstractPaintLayer> highlight_layer_;
  nux::HLayout* layout_;
  nux::HLayout* hint_layout_;
  nux::LayeredLayout* layered_layout_;
  SearchBarSpinner* spinner_;

  nux::Property<bool> caps_lock_on;
  int last_width_;
  int last_height_;

  IconTexture* warning_;
  IconTexture* activator_;
  nux::ObjectPtr<nux::BaseTexture> warning_tooltip_;

  glib::Source::UniquePtr tooltip_timeout_;
  glib::SignalManager sig_manager_;
};

}

#endif
