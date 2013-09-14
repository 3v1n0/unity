/**
 * Copyright : (C) 2006-2012 by Patrick Niklaus, Roi Cohen,
 *              Danny Baumann, Sam Spilsbury
 * Authors: Patrick Niklaus <patrick.niklaus@googlemail.com>
 *          Roi Cohen       <roico.beryl@gmail.com>
 *          Danny Baumann   <maniac@opencompositing.org>
 *          Sam Spilsbury   <smspillaz@gmail.com>
 *          Marco Trevisan <marco.trevisan@canonical.com>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 **/

#ifndef _UNITYSHELL_GLOW_H
#define _UNITYSHELL_GLOW_H

#include <core/core.h>
#include <opengl/opengl.h>

namespace unity
{
namespace glow
{

enum class QuadPos
{
  TOPLEFT = 0,
  TOPRIGHT,
  BOTTOMLEFT,
  BOTTOMRIGHT,
  TOP,
  BOTTOM,
  LEFT,
  RIGHT,
  LAST
};

struct Quads
{
/* Each glow quad contains a 2x2 scale + positional matrix
 * (the 3rd column is not used since that is for matrix skew
 *  operations which we do not care about)
 * and also a CompRect which describes the size and position of
 * the quad on the glow
 */
  struct Quad
  {
    CompRect box;
    GLTexture::Matrix matrix;
  };

  Quad& operator[](QuadPos position) { return inner_vector_[unsigned(position)]; }
  Quad const& operator[](QuadPos position) const { return inner_vector_[unsigned(position)]; }

private:
  Quad inner_vector_[unsigned(QuadPos::LAST)];
};

} // namespace glow
} // namepsace unity

#endif
