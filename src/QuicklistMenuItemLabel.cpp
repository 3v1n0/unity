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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 * Authored by: Jay Taoko <jay.taoko@canonical.com>
 */

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "Nux/Nux.h"
#include "QuicklistMenuItemLabel.h"

QuicklistMenuItemLabel::QuicklistMenuItemLabel (DbusmenuMenuitem* item,
                                                        NUX_FILE_LINE_DECL) :
QuicklistMenuItem (item,
                   NUX_FILE_LINE_PARAM)
{
  _name = g_strdup ("QuicklistMenuItemLabel");
  Initialize (item);
}

QuicklistMenuItemLabel::QuicklistMenuItemLabel (DbusmenuMenuitem* item,
                                                        bool              debug,
                                                        NUX_FILE_LINE_DECL) :
QuicklistMenuItem (item,
                   debug,
                   NUX_FILE_LINE_PARAM)
{
  _name = g_strdup ("QuicklistMenuItemLabel");
  Initialize (item);
}

void
QuicklistMenuItemLabel::Initialize (DbusmenuMenuitem* item)
{
  _item_type  = MENUITEM_TYPE_LABEL;

  if (item)
    _text = g_strdup (dbusmenu_menuitem_property_get (item, DBUSMENU_MENUITEM_PROP_LABEL));
  else
    _text = g_strdup ("Label");

  int textWidth = 1;
  int textHeight = 1;
  GetTextExtents (textWidth, textHeight);
  SetMinimumSize (textWidth + ITEM_INDENT_ABS + 3 * ITEM_MARGIN,
                  textHeight + 2 * ITEM_MARGIN);
}

QuicklistMenuItemLabel::~QuicklistMenuItemLabel ()
{
  if (_normalTexture[0])
    _normalTexture[0]->UnReference ();

  if (_normalTexture[1])
    _normalTexture[1]->UnReference ();

  if (_prelightTexture[0])
    _prelightTexture[0]->UnReference ();

  if (_prelightTexture[1])
    _prelightTexture[1]->UnReference ();
}

void
QuicklistMenuItemLabel::PreLayoutManagement ()
{
  _pre_layout_width = GetBaseWidth ();
  _pre_layout_height = GetBaseHeight ();

  if (_normalTexture[0] == 0)
  {
    UpdateTexture ();
  }

  QuicklistMenuItem::PreLayoutManagement ();
}

long
QuicklistMenuItemLabel::PostLayoutManagement (long layoutResult)
{
  int w = GetBaseWidth();
  int h = GetBaseHeight();

  long result = 0;

  if (_pre_layout_width < w)
    result |= nux::eLargerWidth;
  else if (_pre_layout_width > w)
    result |= nux::eSmallerWidth;
  else
    result |= nux::eCompliantWidth;

  if (_pre_layout_height < h)
    result |= nux::eLargerHeight;
  else if (_pre_layout_height > h)
    result |= nux::eSmallerHeight;
  else
    result |= nux::eCompliantHeight;

  return result;
}

long
QuicklistMenuItemLabel::ProcessEvent (nux::IEvent& event,
                                          long         traverseInfo,
                                          long         processEventInfo)
{
  long result = traverseInfo;

  result = nux::View::PostProcessEvent2 (event, result, processEventInfo);
  return result;
}

void
QuicklistMenuItemLabel::Draw (nux::GraphicsEngine& gfxContext,
                                  bool                 forceDraw)
{
  // Check if the texture have been computed. If they haven't, exit the function.
  if (_normalTexture[0] == NULL)
    return;

  nux::IntrusiveSP<nux::IOpenGLBaseTexture> texture;

  nux::Geometry base = GetGeometry ();

  gfxContext.PushClippingRectangle (base);

  nux::TexCoordXForm texxform;
  texxform.SetWrap (nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);

  gfxContext.GetRenderStates ().SetBlend (true);
  gfxContext.GetRenderStates ().SetPremultipliedBlend (nux::SRC_OVER);

  if (GetEnabled ())
  {
    if (_prelight)
    {
      texture = _prelightTexture[0]->GetDeviceTexture ();
    }
    else
    {
      texture = _normalTexture[0]->GetDeviceTexture ();
    }
    _color = nux::color::White;
  }
  else
  {
    texture = _normalTexture[0]->GetDeviceTexture ();
    _color = nux::Color (0.8f, 0.8f, 0.8f, 1.0f);
  }

  gfxContext.QRP_1Tex (base.x,
                            base.y,
                            base.width,
                            base.height,
                            texture,
                            texxform,
                            _color);

  gfxContext.GetRenderStates().SetBlend (false);

  gfxContext.PopClippingRectangle ();
}

void
QuicklistMenuItemLabel::DrawContent (nux::GraphicsEngine& gfxContext,
                                         bool                 forceDraw)
{
}

void
QuicklistMenuItemLabel::PostDraw (nux::GraphicsEngine& gfxContext,
                                      bool                 forceDraw)
{
}

void
QuicklistMenuItemLabel::UpdateTexture ()
{
  nux::Color transparent = nux::Color (0.0f, 0.0f, 0.0f, 0.0f);
  int        width       = GetBaseWidth ();
  int        height      = GetBaseHeight ();

  _cairoGraphics = new nux::CairoGraphics (CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *cr = _cairoGraphics->GetContext ();

  // draw normal, unchecked version
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_scale (cr, 1.0f, 1.0f);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_set_line_width (cr, 1.0f);

  DrawText (cr, width, height, nux::color::White);

  nux::NBitmapData* bitmap = _cairoGraphics->GetBitmap ();

  if (_normalTexture[0])
    _normalTexture[0]->UnReference ();

  _normalTexture[0] = nux::GetGraphicsDisplay ()->GetGpuDevice ()->CreateSystemCapableTexture ();
  _normalTexture[0]->Update (bitmap);
  delete bitmap;

  // draw active/prelight, unchecked version
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_scale (cr, 1.0f, 1.0f);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_set_line_width (cr, 1.0f);

  _cairoGraphics->DrawRoundedRectangle (cr,
                                        1.0f,
                                        0.5f,
                                        0.5f,
                                        ITEM_CORNER_RADIUS_ABS,
                                        width - 1.0f,
                                        height - 1.0f);
  cairo_fill (cr);

  cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 0.0f);

  DrawText (cr, width, height, transparent);

  bitmap = _cairoGraphics->GetBitmap ();

  if (_prelightTexture[0])
    _prelightTexture[0]->UnReference ();

  _prelightTexture[0] = nux::GetGraphicsDisplay ()->GetGpuDevice ()->CreateSystemCapableTexture ();
  _prelightTexture[0]->Update (bitmap);
  delete bitmap;

  // finally clean up
  delete _cairoGraphics;
}

int QuicklistMenuItemLabel::CairoSurfaceWidth ()
{
  if (_normalTexture[0])
    return _normalTexture[0]->GetWidth ();

  return 0;
}
