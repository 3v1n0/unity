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
  // A single window button
public:
  WindowButton (int type)
  : nux::Button ("X", NUX_TRACKER_LOCATION),
    _normal_tex (NULL),
    _prelight_tex (NULL),
    _pressed_tex (NULL)
  {
    if (type == BUTTON_CLOSE)
      LoadImages ("close");
    else if (type == BUTTON_MINIMISE)
      LoadImages ("minimize");
    else
      LoadImages ("unmaximize");
  }

  ~WindowButton ()
  {
    _normal_tex->UnReference ();
    _prelight_tex->UnReference ();
    _pressed_tex->UnReference ();
  }

  void Draw (nux::GraphicsEngine &GfxContext, bool force_draw)
  {
    nux::Geometry      geo  = GetGeometry ();
    nux::BaseTexture  *tex;
    nux::TexCoordXForm texxform;
 
    GfxContext.PushClippingRectangle (geo);

    if (HasMouseFocus ())
    {
      tex = _pressed_tex;
    }
    else if (IsMouseInside ())
    {
      tex = _prelight_tex;
    }
    else
    {
      tex = _normal_tex;
    }

    GfxContext.GetRenderStates ().SetSeparateBlend (true,
                                                    GL_SRC_ALPHA,
                                                    GL_ONE_MINUS_SRC_ALPHA,
                                                    GL_ONE_MINUS_DST_ALPHA,
                                                    GL_ONE);
    GfxContext.GetRenderStates ().SetColorMask (true, true, true, true);
    if (tex)
      GfxContext.QRP_GLSL_1Tex (geo.x,
                                geo.y,
                                (float)geo.width,
                                (float)geo.height,
                                tex->GetDeviceTexture (),
                                texxform,
                                nux::Color::White);
    GfxContext.GetRenderStates ().SetSeparateBlend (false,
                                                    GL_SRC_ALPHA,
                                                    GL_ONE_MINUS_SRC_ALPHA,
                                                    GL_ONE_MINUS_DST_ALPHA,
                                                    GL_ONE);
    GfxContext.PopClippingRectangle();
  }

  void LoadImages (const char *name)
  {
    //FIXME: We need to somehow be theme aware. Or, at least support the themes
    //       we know and have a good default fallback
    gchar  *filename;
    GError *error = NULL;
    GdkPixbuf *_normal;
    GdkPixbuf *_prelight;
    GdkPixbuf *_pressed;

    filename = g_strdup_printf ("%s/%s.png", AMBIANCE, name);
    _normal = gdk_pixbuf_new_from_file (filename, &error);
    if (error)
    {
      g_warning ("Unable to load window button %s: %s", filename, error->message);
      g_error_free (error);
      error = NULL;
    }
    else
      _normal_tex = nux::CreateTextureFromPixbuf (_normal);
    g_free (filename);
    g_object_unref (_normal);

    filename = g_strdup_printf ("%s/%s_focused_prelight.png", AMBIANCE, name);
    _prelight = gdk_pixbuf_new_from_file (filename, &error);
    if (error)
    {
      g_warning ("Unable to load window button %s: %s", filename, error->message);
      g_error_free (error);
      error = NULL;
    }
    else
      _prelight_tex = nux::CreateTextureFromPixbuf (_prelight);
    g_free (filename);
    g_object_unref (_prelight);

    filename = g_strdup_printf ("%s/%s_focused_pressed.png", AMBIANCE, name);
    _pressed = gdk_pixbuf_new_from_file (filename, &error);
    if (error)
    {
      g_warning ("Unable to load window button %s: %s", name, error->message);
      g_error_free (error);
      error = NULL;
    }
    else
      _pressed_tex = nux::CreateTextureFromPixbuf (_pressed);
    g_free (filename);
    g_object_unref (_pressed);

    if (_normal_tex)
      SetMinimumSize (_normal_tex->GetWidth (), _normal_tex->GetHeight ());
  }

private:
  nux::BaseTexture *_normal_tex;
  nux::BaseTexture *_prelight_tex;
  nux::BaseTexture *_pressed_tex;
};


WindowButtons::WindowButtons ()
: HLayout ("", NUX_TRACKER_LOCATION)
{
  WindowButton *but;

  but = new WindowButton (BUTTON_CLOSE);
  AddView (but, 0, nux::eCenter, nux::eFix);
  but->sigClick.connect (sigc::mem_fun (this, &WindowButtons::OnCloseClicked));
  but->OnMouseEnter.connect (sigc::mem_fun (this, &WindowButtons::RecvMouseEnter));
  but->OnMouseLeave.connect (sigc::mem_fun (this, &WindowButtons::RecvMouseLeave));

  but = new WindowButton (BUTTON_MINIMISE);
  AddView (but, 0, nux::eCenter, nux::eFix);
  but->sigClick.connect (sigc::mem_fun (this, &WindowButtons::OnMinimizeClicked));
  but->OnMouseEnter.connect (sigc::mem_fun (this, &WindowButtons::RecvMouseEnter));
  but->OnMouseLeave.connect (sigc::mem_fun (this, &WindowButtons::RecvMouseLeave));

  but = new WindowButton (BUTTON_UNMAXIMISE);
  AddView (but, 0, nux::eCenter, nux::eFix);
  but->sigClick.connect (sigc::mem_fun (this, &WindowButtons::OnRestoreClicked));
  but->OnMouseEnter.connect (sigc::mem_fun (this, &WindowButtons::RecvMouseEnter));
  but->OnMouseLeave.connect (sigc::mem_fun (this, &WindowButtons::RecvMouseLeave));

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

void WindowButtons::RecvMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  redraw_signal.emit ();
}

void WindowButtons::RecvMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  redraw_signal.emit ();
}


