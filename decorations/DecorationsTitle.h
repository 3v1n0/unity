// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef UNITY_DECORATIONS_TITLE
#define UNITY_DECORATIONS_TITLE

#include "DecorationsWidgets.h"

namespace unity
{
namespace decoration
{

class Title : public TexturedItem
{
public:
  typedef std::shared_ptr<Title> Ptr;

  nux::Property<std::string> text;

  Title();

  void SetX(int);
  int GetNaturalWidth() const;
  int GetNaturalHeight() const;

  void Draw(GLWindow*, GLMatrix const&, GLWindowPaintAttrib const&, CompRegion const&, unsigned mask);

private:
  void OnFontChanged(std::string const&);
  void OnTextChanged(std::string const& new_text);
  void RenderTexture();

  bool render_texture_;
  nux::Size texture_size_;
  nux::Size real_size_;
};

} // decoration namespace
} // unity namespace

#endif // UNITY_DECORATIONS_TITLE
