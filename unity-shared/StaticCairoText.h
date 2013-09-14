// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 * Authored by: Jay Taoko <jay.taoko@canonical.com>
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 */

#ifndef UNITYSHARED_STATICCAIROTEXT_H
#define UNITYSHARED_STATICCAIROTEXT_H

#include <string>

#include <Nux/Nux.h>
#include <Nux/View.h>

#include "unity-shared/Introspectable.h"

namespace unity
{
class Validator;

class StaticCairoText : public nux::View, public unity::debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE (StaticCairoText, nux::View);
public:
  enum EllipsizeState
  {
    NUX_ELLIPSIZE_END,
    NUX_ELLIPSIZE_START,
    NUX_ELLIPSIZE_MIDDLE,
    NUX_ELLIPSIZE_NONE,
  };

  enum AlignState
  {
    NUX_ALIGN_LEFT,
    NUX_ALIGN_CENTRE,
    NUX_ALIGN_RIGHT,
    NUX_ALIGN_TOP = NUX_ALIGN_LEFT,
    NUX_ALIGN_BOTTOM = NUX_ALIGN_RIGHT
  };

  enum UnderlineState
  {
    NUX_UNDERLINE_NONE,
    NUX_UNDERLINE_SINGLE,
    NUX_UNDERLINE_DOUBLE,
    NUX_UNDERLINE_LOW
  };

  StaticCairoText(std::string const& text, NUX_FILE_LINE_PROTO);
  StaticCairoText(std::string const& text, bool escape_text, NUX_FILE_LINE_PROTO);
  ~StaticCairoText();

  void PreLayoutManagement();

  long PostLayoutManagement(long layoutResult);

  void Draw(nux::GraphicsEngine& gfxContext,
            bool             forceDraw);

  void DrawContent(nux::GraphicsEngine& gfxContext,
                   bool             forceDraw);

  // public API
  void SetText(std::string const& text, bool escape_text = false);
  void SetTextAlpha(unsigned int alpha);
  void SetTextColor(nux::Color const& textColor);
  void SetTextEllipsize(EllipsizeState state);
  void SetTextAlignment(AlignState state);
  void SetTextVerticalAlignment(AlignState state);
  void SetFont(std::string const& font);
  std::string GetFont();
  void SetUnderline(UnderlineState underline);
  void SetLines(int maximum_lines);
  void SetLineSpacing(float line_spacing);

  std::string GetText() const;
  nux::Color GetTextColor() const;

  int GetLineCount() const;
  int GetBaseline() const;

  void GetTextExtents(int& width, int& height) const;
  nux::Size GetTextExtents() const;

  sigc::signal<void, StaticCairoText*> sigTextChanged;
  sigc::signal<void, StaticCairoText*> sigTextColorChanged;
  sigc::signal<void, StaticCairoText*> sigFontChanged;

  void SetAcceptKeyNavFocus(bool accept);

  void SetMaximumSize(int w, int h);
  void SetMaximumWidth(int w);

  static std::string GetEscapedText(std::string const& text);

protected:
  // Key navigation
  virtual bool AcceptKeyNavFocus();

  std::vector<unsigned> GetTextureStartIndices();
  std::vector<unsigned> GetTextureEndIndices();

  // From debug::Introspectable
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  struct Impl;
  Impl* pimpl;
};
}

#endif // STATICCAIROTEXT_H
