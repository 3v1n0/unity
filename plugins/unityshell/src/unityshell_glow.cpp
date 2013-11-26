/**
 * Copyright : (C) 2006-2012 by Patrick Niklaus, Roi Cohen,
 *                Danny Baumann, Sam Spilsbury
 * Authors: Patrick Niklaus <patrick.niklaus@googlemail.com>
 *      Roi Cohen     <roico.beryl@gmail.com>
 *      Danny Baumann   <maniac@opencompositing.org>
 *      Sam Spilsbury   <smspillaz@gmail.com>
 *      Marco Trevisan <marco.trevisan@canonical.com>
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

#include <Nux/Nux.h>
#include "unityshell.h"
#include "glow_texture.h"
#include "unityshell_glow.h"

namespace unity
{

/*
 * UnityWindow::paintGlow
 *
 * Takes our glow texture, stretches the appropriate positions in the glow texture,
 * adds those geometries (so plugins like wobby and deform this texture correctly)
 * and then draws the glow texture with this geometry (plugins like wobbly and friends
 * will automatically deform the texture based on our set geometry)
 */

void
UnityWindow::paintGlow(GLMatrix const& transform, GLWindowPaintAttrib const& attrib,
                       glow::Quads const& glow_quads, GLTexture::List const& outline_texture,
                       nux::Color const& color, unsigned mask)
{
  GLushort colorData[4];
  colorData[0] = color.red * 0xffff;
  colorData[1] = color.green * 0xffff;
  colorData[2] = color.blue * 0xffff;
  colorData[3] = color.alpha * 0xffff;

  gWindow->vertexBuffer()->begin ();

  /* There are 8 glow parts of the glow texture which we wish to paint
   * separately with different transformations
   */
  for (unsigned i = 0; i < unsigned(glow::QuadPos::LAST); ++i)
  {
    /* Using precalculated quads here */
    glow::Quads::Quad const& quad = glow_quads[static_cast<glow::QuadPos>(i)];

    if (quad.box.x1() < quad.box.x2() && quad.box.y1() < quad.box.y2())
    {
      GLTexture::MatrixList matl = { quad.matrix };

      /* Add color data for all 6 vertices of the quad */
      for (int n = 0; n < 6; n++)
        gWindow->vertexBuffer()->addColors(1, colorData);

      CompRegion reg(quad.box);
      gWindow->glAddGeometry(matl, reg, reg);
    }
  }

  if (gWindow->vertexBuffer()->end())
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* we use PAINT_WINDOW_TRANSFORMED_MASK here to force
    the usage of a good texture filter */
    for (GLTexture *tex : outline_texture)
    {
      mask |= PAINT_WINDOW_BLEND_MASK | PAINT_WINDOW_TRANSLUCENT_MASK | PAINT_WINDOW_TRANSFORMED_MASK;
      gWindow->glDrawTexture(tex, transform, attrib, mask);
    }

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    GLScreen::get(screen)->setTexEnvMode (GL_REPLACE);
  }
}

/*
 * UnityWindow::computeGlowQuads
 *
 * This function computures the matrix transformation required for each
 * part of the glow texture which we wish to stretch to some rectangular
 * dimentions
 *
 * There are eight quads different parts of the texture which we wish to
 * paint here, the 4 sides and four corners, eg:
 *
 *                  ------------------
 *                  | 1 |   4    | 6 |
 * -------------    ------------------
 * | 1 | 4 | 6 |    |   |        |   |
 * -------------    |   |        |   |
 * | 2 | n | 7 | -> | 2 |   n    | 7 |
 * -------------    |   |        |   |
 * | 3 | 5 | 8 |    |   |        |   |
 * -------------    ------------------
 *                  | 3 |   5    | 8 |
 *                  ------------------
 *
 * In this example here, 2, 4, 5 and 7 are stretched, and the matrices for
 * each quad rect adjusted accordingly for it's size compared to the original
 * texture size.
 *
 * When we are adjusting the matrices here, the initial size of each corner has
 * a size of of "1.0f", so according to 2x2 matrix rules,
 * while you will see here that matrix->xx is (1 / glowSize)
 * where glowSize is the size the user specifies they want their glow to extend.
 * (likewise, matrix->yy is adjusted similarly for corners and for top/bottom)
 *
 * matrix->x0 and matrix->y0 here are set to be the top left edge of the rect
 * adjusted by the matrix scale factor (matrix->xx and matrix->yy)
 *
 */
glow::Quads UnityWindow::computeGlowQuads(nux::Geometry const& geo, GLTexture::List const& texture, int glow_size)
{
  glow::Quads glow_quads;

  if (texture.empty())
    return glow_quads;

  int x1, x2, y1, y2;
  int glow_offset;
  GLTexture::Matrix const& matrix = texture.front()->matrix();

  CompRect *box;
  GLTexture::Matrix *quadMatrix;

  glow_size = glow_size * texture::GLOW_SIZE / (texture::GLOW_SIZE - texture::GLOW_OFFSET);
  glow_offset = (glow_size * texture::GLOW_OFFSET / texture::GLOW_SIZE) + 1;

  /* Top left corner */
  box = &glow_quads[glow::QuadPos::TOPLEFT].box;
  glow_quads[glow::QuadPos::TOPLEFT].matrix = matrix;
  quadMatrix = &glow_quads[glow::QuadPos::TOPLEFT].matrix;

  /* Set the desired rect dimentions
   * for the part of the glow we are painting */

  x1 = geo.x - glow_size + glow_offset;
  y1 = geo.y - glow_size + glow_offset;

  /* 2x2 Matrix here, adjust both x and y scale factors
   * and the x and y position
   *
   * Scaling both parts of the texture in a positive direction
   * here (left to right top to bottom)
   *
   * The base position (x0 and y0) here requires us to move backwards
   * on the x and y dimentions by the calculated rect dimentions
   * multiplied by the scale factors
   */

  quadMatrix->xx = 1.0f / glow_size;
  quadMatrix->yy = 1.0f / glow_size;
  quadMatrix->x0 = -(x1 * quadMatrix->xx);
  quadMatrix->y0 = -(y1 * quadMatrix->yy);

  x2 = std::min<int>(geo.x + glow_offset, geo.x + (geo.width / 2));
  y2 = std::min<int>(geo.y + glow_offset, geo.y + (geo.height / 2));

  box->setGeometry(x1, y1, x2 - x1, y2 - y1);

  /* Top right corner */
  box = &glow_quads[glow::QuadPos::TOPRIGHT].box;
  glow_quads[glow::QuadPos::TOPRIGHT].matrix = matrix;
  quadMatrix = &glow_quads[glow::QuadPos::TOPRIGHT].matrix;

  /* Set the desired rect dimentions
   * for the part of the glow we are painting */

  x1 = geo.x + geo.width - glow_offset;
  y1 = geo.y - glow_size + glow_offset;
  x2 = geo.x + geo.width + glow_size - glow_offset;

  /*
   * 2x2 Matrix here, adjust both x and y scale factors
   * and the x and y position
   *
   * Scaling the y part of the texture in a positive direction
   * and the x part in a negative direction here
   * (right to left top to bottom)
   *
   * The base position (x0 and y0) here requires us to move backwards
   * on the y dimention and forwards on x by the calculated rect dimentions
   * multiplied by the scale factors (since we are moving forward on x we
   * need the inverse of that which is 1 - x1 * xx
   */

  quadMatrix->xx = -1.0f / glow_size;
  quadMatrix->yy = 1.0f / glow_size;
  quadMatrix->x0 = 1.0 - (x1 * quadMatrix->xx);
  quadMatrix->y0 = -(y1 * quadMatrix->yy);

  x1 = std::max<int>(geo.x + geo.width - glow_offset, geo.x + (geo.width / 2));
  y2 = std::min<int>(geo.y + glow_offset, geo.y + (geo.height / 2));

  box->setGeometry(x1, y1, x2 - x1, y2 - y1);

  /* Bottom left corner */
  box = &glow_quads[glow::QuadPos::BOTTOMLEFT].box;
  glow_quads[glow::QuadPos::BOTTOMLEFT].matrix = matrix;
  quadMatrix = &glow_quads[glow::QuadPos::BOTTOMLEFT].matrix;

  x1 = geo.x - glow_size + glow_offset;
  y1 = geo.y + geo.height - glow_offset;
  x2 = geo.x + glow_offset;
  y2 = geo.y + geo.height + glow_size - glow_offset;

  /* 2x2 Matrix here, adjust both x and y scale factors
   * and the x and y position
   *
   * Scaling the x part of the texture in a positive direction
   * and the y part in a negative direction here
   * (left to right bottom to top)
   *
   * The base position (x0 and y0) here requires us to move backwards
   * on the x dimention and forwards on y by the calculated rect dimentions
   * multiplied by the scale factors (since we are moving forward on x we
   * need the inverse of that which is 1 - y1 * yy
   */

  quadMatrix->xx = 1.0f / glow_size;
  quadMatrix->yy = -1.0f / glow_size;
  quadMatrix->x0 = -(x1 * quadMatrix->xx);
  quadMatrix->y0 = 1.0f - (y1 * quadMatrix->yy);

  y1 = std::max<int>(geo.y + geo.height - glow_offset, geo.y + (geo.height / 2));
  x2 = std::min<int>(x2, geo.x + (geo.width / 2));

  box->setGeometry(x1, y1, x2 - x1, y2 - y1);

  /* Bottom right corner */
  box = &glow_quads[glow::QuadPos::BOTTOMRIGHT].box;
  glow_quads[glow::QuadPos::BOTTOMRIGHT].matrix = matrix;
  quadMatrix = &glow_quads[glow::QuadPos::BOTTOMRIGHT].matrix;

  x1 = geo.x + geo.width - glow_offset;
  y1 = geo.y + geo.height - glow_offset;
  x2 = geo.x + geo.width + glow_size - glow_offset;
  y2 = geo.y + geo.height + glow_size - glow_offset;

  /* 2x2 Matrix here, adjust both x and y scale factors
   * and the x and y position
   *
   * Scaling the both parts of the texture in a negative direction
   * (right to left bottom to top)
   *
   * The base position (x0 and y0) here requires us to move forwards
   * on both dimentions by the calculated rect dimentions
   * multiplied by the scale factors
   */

  quadMatrix->xx = -1.0f / glow_size;
  quadMatrix->yy = -1.0f / glow_size;
  quadMatrix->x0 = 1.0 - (x1 * quadMatrix->xx);
  quadMatrix->y0 = 1.0 - (y1 * quadMatrix->yy);

  x1 = std::max<int>(geo.x + geo.width - glow_offset, geo.x + (geo.width / 2));
  y1 = std::max<int>(geo.y + geo.height - glow_offset, geo.y + (geo.height / 2));

  box->setGeometry(x1, y1, x2 - x1, y2 - y1);

  /* Top edge */
  box = &glow_quads[glow::QuadPos::TOP].box;
  glow_quads[glow::QuadPos::TOP].matrix = matrix;
  quadMatrix = &glow_quads[glow::QuadPos::TOP].matrix;

  x1 = geo.x + glow_offset;
  y1 = geo.y - glow_size + glow_offset;
  x2 = geo.x + geo.width - glow_offset;
  y2 = geo.y + glow_offset;

  /* 2x2 Matrix here, adjust both x and y scale factors
   * and the x and y position
   *
   * No need to scale the x part of the texture here, but we
   * are scaling on the y part in a positive direciton
   *
   * The base position (y0) here requires us to move backwards
   * on the x dimention and forwards on y by the calculated rect dimentions
   * multiplied by the scale factors
   */

  quadMatrix->xx = 0.0f;
  quadMatrix->yy = 1.0f / glow_size;
  quadMatrix->x0 = 1.0;
  quadMatrix->y0 = -(y1 * quadMatrix->yy);

  box->setGeometry(x1, y1, x2 - x1, y2 - y1);

  /* Bottom edge */
  box = &glow_quads[glow::QuadPos::BOTTOM].box;
  glow_quads[glow::QuadPos::BOTTOM].matrix = matrix;
  quadMatrix = &glow_quads[glow::QuadPos::BOTTOM].matrix;

  x1 = geo.x + glow_offset;
  y1 = geo.y + geo.height - glow_offset;
  x2 = geo.x + geo.width - glow_offset;
  y2 = geo.y + geo.height + glow_size - glow_offset;

  /* 2x2 Matrix here, adjust both x and y scale factors
   * and the x and y position
   *
   * No need to scale the x part of the texture here, but we
   * are scaling on the y part in a negative direciton
   *
   * The base position (y0) here requires us to move forwards
   * on y by the calculated rect dimentions
   * multiplied by the scale factors
   */

  quadMatrix->xx = 0.0f;
  quadMatrix->yy = -1.0f / glow_size;
  quadMatrix->x0 = 1.0;
  quadMatrix->y0 = 1.0 - (y1 * quadMatrix->yy);

  box->setGeometry(x1, y1, x2 - x1, y2 - y1);

  /* Left edge */
  box = &glow_quads[glow::QuadPos::LEFT].box;
  glow_quads[glow::QuadPos::LEFT].matrix = matrix;
  quadMatrix = &glow_quads[glow::QuadPos::LEFT].matrix;

  x1 = geo.x - glow_size + glow_offset;
  y1 = geo.y + glow_offset;
  x2 = geo.x + glow_offset;
  y2 = geo.y + geo.height - glow_offset;

  /* 2x2 Matrix here, adjust both x and y scale factors
   * and the x and y position
   *
   * No need to scale the y part of the texture here, but we
   * are scaling on the x part in a positive direciton
   *
   * The base position (x0) here requires us to move backwards
   * on x by the calculated rect dimentions
   * multiplied by the scale factors
   */

  quadMatrix->xx = 1.0f / glow_size;
  quadMatrix->yy = 0.0f;
  quadMatrix->x0 = -(x1 * quadMatrix->xx);
  quadMatrix->y0 = 1.0;

  box->setGeometry(x1, y1, x2 - x1, y2 - y1);

  /* Right edge */
  box = &glow_quads[glow::QuadPos::RIGHT].box;
  glow_quads[glow::QuadPos::RIGHT].matrix = matrix;
  quadMatrix = &glow_quads[glow::QuadPos::RIGHT].matrix;

  x1 = geo.x + geo.width - glow_offset;
  y1 = geo.y + glow_offset;
  x2 = geo.x + geo.width + glow_size - glow_offset;
  y2 = geo.y + geo.height - glow_offset;

  /* 2x2 Matrix here, adjust both x and y scale factors
   * and the x and y position
   *
   * No need to scale the y part of the texture here, but we
   * are scaling on the x part in a negative direciton
   *
   * The base position (x0) here requires us to move forwards
   * on x by the calculated rect dimentions
   * multiplied by the scale factors
   */

  quadMatrix->xx = -1.0f / glow_size;
  quadMatrix->yy = 0.0f;
  quadMatrix->x0 = 1.0 - (x1 * quadMatrix->xx);
  quadMatrix->y0 = 1.0;

  box->setGeometry(x1, y1, x2 - x1, y2 - y1);

  return glow_quads;
}

} // Unity namespace
