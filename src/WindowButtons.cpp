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

#include "Nux/Nux.h"
#include "Nux/HLayout.h"
#include "Nux/VLayout.h"
#include "Nux/Button.h"

#include "NuxGraphics/GLThread.h"
#include "Nux/BaseWindow.h"
#include "Nux/WindowCompositor.h"

#include "WindowButtons.h"

#include <glib.h>


// FIXME: This will be all automatic in the future
#define AMBIANCE "/usr/share/themes/Ambiance/metacity-1"

enum
{
  BUTTON_CLOSE=0,
  BUTTON_MINIMISE,
  BUTTON_UNMAXIMISE
};

class WindowButton : public nux::Button
{
public:
  WindowButton (int type)
  : nux::Button ("X", NUX_TRACKER_LOCATION)
  {
    if (type == BUTTON_CLOSE)
      LoadImages ("close");
    else if (type == BUTTON_MINIMISE)
      LoadImages ("minimize");
    else
      LoadImages ("unmaximize");

    LoadTextures ();
  }

  ~WindowButton ()
  {
    delete _normal_layer;
    delete _prelight_layer;
    delete _pressed_layer;
  }

  void Draw (nux::GraphicsEngine &GfxContext, bool force_draw)
  {
    nux::Geometry geo = GetGeometry ();
    nux::AbstractPaintLayer *alayer;
  
    GfxContext.PushClippingRectangle (geo);
    
    nux::ROPConfig rop; 
    rop.Blend = true;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
 
    nux::ColorLayer layer (nux::Color (0x00000000), true, rop);
    gPainter.PushDrawLayer (GfxContext, GetGeometry (), &layer);

    if (HasMouseFocus ())
    {
      alayer = _pressed_layer;
      g_debug ("pressed");
    }
    else if (IsMouseInside ())
    {
      alayer = _prelight_layer;
      g_debug ("prelight");
    }
    else
    {
      alayer = _normal_layer;
      g_debug ("normal");
    }

    gPainter.PushDrawLayer (GfxContext, GetGeometry (), alayer);

    gPainter.PopBackground ();
 
    GfxContext.PopClippingRectangle();
  }

  void LoadImages (const char *name)
  {
    gchar  *filename;
    GError *error = NULL;

    filename = g_strdup_printf ("%s/%s.png", AMBIANCE, name);
    _normal = gdk_pixbuf_new_from_file (filename, &error);
    if (error)
    {
      g_warning ("Unable to load window button %s: %s", filename, error->message);
      g_error_free (error);
      error = NULL;
    }
    g_free (filename);

    filename = g_strdup_printf ("%s/%s_focused_prelight.png", AMBIANCE, name);
    _prelight = gdk_pixbuf_new_from_file (filename, &error);
    if (error)
    {
      g_warning ("Unable to load window button %s: %s", filename, error->message);
      g_error_free (error);
      error = NULL;
    }
    g_free (filename);

    filename = g_strdup_printf ("%s/%s_focused_pressed.png", AMBIANCE, name);
    _pressed = gdk_pixbuf_new_from_file (filename, &error);
    if (error)
    {
      g_warning ("Unable to load window button %s: %s", name, error->message);
      g_error_free (error);
      error = NULL;
    }
    g_free (filename);
  }

  void LoadTextures ()
  {
    nux::BaseTexture *texture;   
    nux::ROPConfig rop; 
    nux::TexCoordXForm texxform;

    rop.Blend = true;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

    texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);
    texxform.SetWrap (nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
    
    texture = nux::CreateTextureFromPixbuf (_normal);
    _normal_layer = new nux::TextureLayer (texture->GetDeviceTexture(),
                                          texxform,
                                          nux::Color::White,
                                          false, 
                                          rop);
    texture->UnReference ();

    texture = nux::CreateTextureFromPixbuf (_prelight);
    _prelight_layer = new nux::TextureLayer (texture->GetDeviceTexture(),
                                             texxform,
                                             nux::Color::White,
                                             false, 
                                             rop);
    texture->UnReference ();
    
    texture = nux::CreateTextureFromPixbuf (_pressed);
    _pressed_layer = new nux::TextureLayer (texture->GetDeviceTexture(),
                                            texxform,
                                            nux::Color::White,
                                            false, 
                                            rop);

    SetMinimumSize (texture->GetWidth (), texture->GetHeight ());

    texture->UnReference ();

    g_object_unref (_normal);
    g_object_unref (_prelight);
    g_object_unref (_pressed);
  }

private:
  GdkPixbuf *_normal;
  GdkPixbuf *_prelight;
  GdkPixbuf *_pressed;

  nux::AbstractPaintLayer *_normal_layer;
  nux::AbstractPaintLayer *_prelight_layer;
  nux::AbstractPaintLayer *_pressed_layer;
};


WindowButtons::WindowButtons ()
{
  WindowButton *but;

  but = new WindowButton (BUTTON_CLOSE);
  AddView (but, 0, nux::eCenter, nux::eFix);
  but->sigClick.connect (sigc::mem_fun (this, &WindowButtons::OnCloseClicked));

  but = new WindowButton (BUTTON_MINIMISE);
  AddView (but, 0, nux::eCenter, nux::eFix);
  but->sigClick.connect (sigc::mem_fun (this, &WindowButtons::OnMinimizeClicked));

  but = new WindowButton (BUTTON_UNMAXIMISE);
  AddView (but, 0, nux::eCenter, nux::eFix);
  but->sigClick.connect (sigc::mem_fun (this, &WindowButtons::OnRestoreClicked));
  
  SetContentDistribution (nux::eStackLeft);
}


WindowButtons::~WindowButtons ()
{
}

void
WindowButtons::OnCloseClicked ()
{
  close_clicked.emit ();
}

void
WindowButtons::OnMinimizeClicked ()
{
  minimize_clicked.emit ();
}

void
WindowButtons::OnRestoreClicked ()
{
  restore_clicked.emit ();
}

const gchar *
WindowButtons::GetName ()
{
  return "window-buttons";
}

const gchar *
WindowButtons::GetChildsName ()
{
  return "";
}

void
WindowButtons::AddProperties (GVariantBuilder *builder)
{
  nux::Geometry geo = GetGeometry ();

  /* Now some props from ourselves */
  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));
}
