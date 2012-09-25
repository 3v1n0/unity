/**
 *
 * Compiz group plugin
 *
 * glow.cpp
 *
 * Copyright : (C) 2006-2010 by Patrick Niklaus, Roi Cohen,
 *                Danny Baumann, Sam Spilsbury
 * Authors: Patrick Niklaus <patrick.niklaus@googlemail.com>
 *      Roi Cohen     <roico.beryl@gmail.com>
 *      Danny Baumann   <maniac@opencompositing.org>
 *      Sam Spilsbury   <smspillaz@gmail.com>
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
#include "group_glow.h"
#include "glow.h"

#define WIN_REAL_X(w) (w->borderRect().x())
#define WIN_REAL_Y(w) (w->borderRect().y())
#define WIN_REAL_WIDTH(w) (w->borderRect().width())
#define WIN_REAL_HEIGHT(w) (w->borderRect().height())

const GlowTextureProperties glowTextureProperties = {
  /* GlowTextureRectangular */
  glowTexRect, 32, 21
};

/*
 * GroupWindow::paintGlow
 *
 * Takes our glow texture, stretches the appropriate positions in the glow texture,
 * adds those geometries (so plugins like wobby and deform this texture correctly)
 * and then draws the glow texture with this geometry (plugins like wobbly and friends
 * will automatically deform the texture based on our set geometry)
 */

void
UnityWindow::paintGlow (const GLMatrix      &transform,
             const GLWindowPaintAttrib &attrib,
             const CompRegion      &paintRegion,
             GLTexture::List const& outline_texture,
             const GLushort *selColorData,
             unsigned int         mask)
{

  CompRegion    reg;
  int       i;
  GLushort    colorData[4];
  float       alpha = (float) selColorData[3] / 65535.0f;

  /* Premultiply color */
  colorData[0] = selColorData[0] * alpha;
  colorData[1] = selColorData[1] * alpha;
  colorData[2] = selColorData[2] * alpha;
  colorData[3] = selColorData[3];

  gWindow->vertexBuffer ()->begin ();

  /* There are 8 glow parts of the glow texture which we wish to paint
   * separately with different transformations
   */
  for (i = 0; i < NUM_GLOWQUADS; i++)
  {
    /* Using precalculated quads here */
    reg = CompRegion (mGlowQuads[i].mBox);

    if (reg.boundingRect ().x1 () < reg.boundingRect ().x2 () &&
      reg.boundingRect ().y1 () < reg.boundingRect ().y2 ())
    {
      GLTexture::MatrixList matl;
      reg = CompRegion (reg.boundingRect ().x1 (),
                reg.boundingRect ().y1 (),
                reg.boundingRect ().width (),
                reg.boundingRect ().height ());

      matl.push_back (mGlowQuads[i].mMatrix);
      /* Add color data for all 6 vertices of the quad */
      for (int n = 0; n < 6; n++)
        gWindow->vertexBuffer ()->addColors (1, colorData);
      gWindow->glAddGeometry (matl, reg, reg);
    }
  }

  if (gWindow->vertexBuffer ()->end ())
  {
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* we use PAINT_WINDOW_TRANSFORMED_MASK here to force
    the usage of a good texture filter */
    for (GLTexture *tex : outline_texture)
    {
      gWindow->glDrawTexture (tex, transform, attrib, mask |
                  PAINT_WINDOW_BLEND_MASK     |
                  PAINT_WINDOW_TRANSLUCENT_MASK |
                  PAINT_WINDOW_TRANSFORMED_MASK);
    }

    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    GLScreen::get (screen)->setTexEnvMode (GL_REPLACE);
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
 * the scale factor is the inverse of the size of the glow (which explains
 * while you will see here that matrix->xx is (1 / glowSize)
 * where glowSize is the size the user specifies they want their glow to extend.
 * (likewise, matrix->yy is adjusted similarly for corners and for top/bottom)
 *
 * matrix->x0 and matrix->y0 here are set to be the top left edge of the rect
 * adjusted by the matrix scale factor (matrix->xx and matrix->yy)
 *
 */
void
UnityWindow::computeGlowQuads (GLTexture::Matrix *matrix, double aspect)
{
  CompRect      *box;
  int         x1, x2, y1, y2;
  GLTexture::Matrix *quadMatrix;
  float         glowSize, glowOffset;
  CompWindow    *w = window;

  /* Passing NULL to this function frees the glow quads
   * (so the window is not painted with glow) */

  if (matrix)
  {
    if (!mGlowQuads)
      mGlowQuads = new GlowQuad[NUM_GLOWQUADS];
    if (!mGlowQuads)
      return;
  }
  else
  {
    if (mGlowQuads)
    {
      delete[] mGlowQuads;
      mGlowQuads = NULL;
    }
    return;
  }

  glowSize = 30 / aspect;
  glowOffset = (glowSize * glowTextureProperties.glowOffset /
                           glowTextureProperties.textureSize) + 1;

  /* Top left corner */
  box = &mGlowQuads[GLOWQUAD_TOPLEFT].mBox;
  mGlowQuads[GLOWQUAD_TOPLEFT].mMatrix = *matrix;
  quadMatrix = &mGlowQuads[GLOWQUAD_TOPLEFT].mMatrix;

  /* Set the desired rect dimentions
   * for the part of the glow we are painting */

  x1 = WIN_REAL_X (w) - glowSize + glowOffset;
  y1 = WIN_REAL_Y (w) - glowSize + glowOffset;

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

  quadMatrix->xx = 1.0f / glowSize;
  quadMatrix->yy = 1.0f / glowSize;
  quadMatrix->x0 = -(x1 * quadMatrix->xx);
  quadMatrix->y0 = -(y1 * quadMatrix->yy);

  x2 = MIN (WIN_REAL_X (w) + glowOffset,
        WIN_REAL_X (w) + (WIN_REAL_WIDTH (w) / 2));
  y2 = MIN (WIN_REAL_Y (w) + glowOffset,
        WIN_REAL_Y (w) + (WIN_REAL_HEIGHT (w) / 2));

  *box = CompRect (x1, y1, x2 - x1, y2 - y1);

  /* Top right corner */
  box = &mGlowQuads[GLOWQUAD_TOPRIGHT].mBox;
  mGlowQuads[GLOWQUAD_TOPRIGHT].mMatrix = *matrix;
  quadMatrix = &mGlowQuads[GLOWQUAD_TOPRIGHT].mMatrix;

  /* Set the desired rect dimentions
   * for the part of the glow we are painting */

  x1 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset;
  y1 = WIN_REAL_Y (w) - glowSize + glowOffset;
  x2 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) + glowSize - glowOffset;

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

  quadMatrix->xx = -1.0f / glowSize;
  quadMatrix->yy = 1.0f / glowSize;
  quadMatrix->x0 = 1.0 - (x1 * quadMatrix->xx);
  quadMatrix->y0 = -(y1 * quadMatrix->yy);

  x1 = MAX (WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset,
        WIN_REAL_X (w) + (WIN_REAL_WIDTH (w) / 2));
  y2 = MIN (WIN_REAL_Y (w) + glowOffset,
        WIN_REAL_Y (w) + (WIN_REAL_HEIGHT (w) / 2));

  *box = CompRect (x1, y1, x2 - x1, y2 - y1);

  /* Bottom left corner */
  box = &mGlowQuads[GLOWQUAD_BOTTOMLEFT].mBox;
  mGlowQuads[GLOWQUAD_BOTTOMLEFT].mMatrix = *matrix;
  quadMatrix = &mGlowQuads[GLOWQUAD_BOTTOMLEFT].mMatrix;

  x1 = WIN_REAL_X (w) - glowSize + glowOffset;
  y1 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset;
  x2 = WIN_REAL_X (w) + glowOffset;
  y2 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) + glowSize - glowOffset;

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

  quadMatrix->xx = 1.0f / glowSize;
  quadMatrix->yy = -1.0f / glowSize;
  quadMatrix->x0 = -(x1 * quadMatrix->xx);
  quadMatrix->y0 = 1.0f - (y1 * quadMatrix->yy);

  y1 = MAX (WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset,
        WIN_REAL_Y (w) + (WIN_REAL_HEIGHT (w) / 2));
  x2 = MIN (WIN_REAL_X (w) + glowOffset,
        WIN_REAL_X (w) + (WIN_REAL_WIDTH (w) / 2));

  *box = CompRect (x1, y1, x2 - x1, y2 - y1);

  /* Bottom right corner */
  box = &mGlowQuads[GLOWQUAD_BOTTOMRIGHT].mBox;
  mGlowQuads[GLOWQUAD_BOTTOMRIGHT].mMatrix = *matrix;
  quadMatrix = &mGlowQuads[GLOWQUAD_BOTTOMRIGHT].mMatrix;

  x1 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset;
  y1 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset;
  x2 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) + glowSize - glowOffset;
  y2 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) + glowSize - glowOffset;

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

  quadMatrix->xx = -1.0f / glowSize;
  quadMatrix->yy = -1.0f / glowSize;
  quadMatrix->x0 = 1.0 - (x1 * quadMatrix->xx);
  quadMatrix->y0 = 1.0 - (y1 * quadMatrix->yy);

  x1 = MAX (WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset,
        WIN_REAL_X (w) + (WIN_REAL_WIDTH (w) / 2));
  y1 = MAX (WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset,
        WIN_REAL_Y (w) + (WIN_REAL_HEIGHT (w) / 2));

  *box = CompRect (x1, y1, x2 - x1, y2 - y1);

  /* Top edge */
  box = &mGlowQuads[GLOWQUAD_TOP].mBox;
  mGlowQuads[GLOWQUAD_TOP].mMatrix = *matrix;
  quadMatrix = &mGlowQuads[GLOWQUAD_TOP].mMatrix;

  x1 = WIN_REAL_X (w) + glowOffset;
  y1 = WIN_REAL_Y (w) - glowSize + glowOffset;
  x2 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset;
  y2 = WIN_REAL_Y (w) + glowOffset;

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
  quadMatrix->yy = 1.0f / glowSize;
  quadMatrix->x0 = 1.0;
  quadMatrix->y0 = -(y1 * quadMatrix->yy);

  *box = CompRect (x1, y1, x2 - x1, y2 - y1);

  /* Bottom edge */
  box = &mGlowQuads[GLOWQUAD_BOTTOM].mBox;
  mGlowQuads[GLOWQUAD_BOTTOM].mMatrix = *matrix;
  quadMatrix = &mGlowQuads[GLOWQUAD_BOTTOM].mMatrix;

  x1 = WIN_REAL_X (w) + glowOffset;
  y1 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset;
  x2 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset;
  y2 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) + glowSize - glowOffset;

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
  quadMatrix->yy = -1.0f / glowSize;
  quadMatrix->x0 = 1.0;
  quadMatrix->y0 = 1.0 - (y1 * quadMatrix->yy);

  *box = CompRect (x1, y1, x2 - x1, y2 - y1);

  /* Left edge */
  box = &mGlowQuads[GLOWQUAD_LEFT].mBox;
  mGlowQuads[GLOWQUAD_LEFT].mMatrix = *matrix;
  quadMatrix = &mGlowQuads[GLOWQUAD_LEFT].mMatrix;

  x1 = WIN_REAL_X (w) - glowSize + glowOffset;
  y1 = WIN_REAL_Y (w) + glowOffset;
  x2 = WIN_REAL_X (w) + glowOffset;
  y2 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset;

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

  quadMatrix->xx = 1.0f / glowSize;
  quadMatrix->yy = 0.0f;
  quadMatrix->x0 = -(x1 * quadMatrix->xx);
  quadMatrix->y0 = 1.0;

  *box = CompRect (x1, y1, x2 - x1, y2 - y1);

  /* Right edge */
  box = &mGlowQuads[GLOWQUAD_RIGHT].mBox;
  mGlowQuads[GLOWQUAD_RIGHT].mMatrix = *matrix;
  quadMatrix = &mGlowQuads[GLOWQUAD_RIGHT].mMatrix;

  x1 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset;
  y1 = WIN_REAL_Y (w) + glowOffset;
  x2 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) + glowSize - glowOffset;
  y2 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset;

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

  quadMatrix->xx = -1.0f / glowSize;
  quadMatrix->yy = 0.0f;
  quadMatrix->x0 = 1.0 - (x1 * quadMatrix->xx);
  quadMatrix->y0 = 1.0;

  *box = CompRect (x1, y1, x2 - x1, y2 - y1);
}
