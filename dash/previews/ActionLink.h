// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Manuel de la Pena <manuel.delapena@canonical.com>
 *
 */

#ifndef ACTIONLINK_H
#define ACTIONLINK_H

#include <Nux/Nux.h>
#include <Nux/CairoWrapper.h>
#include <Nux/AbstractButton.h>
#include "unity-shared/Introspectable.h"
#include "unity-shared/StaticCairoText.h"


namespace unity
{
class IconTexture;

namespace dash
{

class ActionLink : public nux::AbstractButton, public debug::Introspectable
{
public:
  ActionLink(std::string const& action_hint, std::string const& label, NUX_FILE_LINE_PROTO);

  sigc::signal<void, ActionLink*, std::string const&> activate;

  nux::RWProperty<StaticCairoText::AlignState> text_aligment;
  nux::RWProperty<StaticCairoText::UnderlineState> underline_state;
  nux::RWProperty<std::string> font_hint;

  void Activate() {}
  void Deactivate() {}

  virtual bool AcceptKeyNavFocus() const { return true; }

  std::string GetLabel() const;
  std::string GetExtraText() const;

protected:
  nux::ObjectPtr<StaticCairoText> static_text_;

  int GetLinkAlpha(nux::ButtonVisualState state, bool keyboardFocus);

  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {}
  void RecvClick(int x, int y, unsigned long button_flags, unsigned long key_flags);

  void Init();

  void BuildLayout(std::string const& label);

  // From debug::Introspectable
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  // this methods/vars could be private but are protected to make testing
  //  easier
  bool set_aligment(StaticCairoText::AlignState aligment);
  StaticCairoText::AlignState get_aligment();

  bool set_underline(StaticCairoText::UnderlineState underline);
  StaticCairoText::UnderlineState get_underline();

  bool set_font_hint(std::string font_hint);
  std::string get_font_hint();

  std::string action_hint_;
  std::string font_hint_;
  StaticCairoText::AlignState aligment_;
  StaticCairoText::UnderlineState underline_;
private:
  typedef std::unique_ptr<nux::CairoWrapper> NuxCairoPtr;


};

} // namespace dash
} // namespace unity

#endif // ACTIONLINK_H
