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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "config.h"

#include "Nux/Nux.h"

#include "NuxGraphics/GLThread.h"

#include "ubus-server.h"
#include "UBusMessages.h"

#include "PanelHomeButton.h"

#include <glib.h>
#include <pango/pangocairo.h>
#include <gtk/gtk.h>

#define PANEL_HEIGHT 24
#define BUTTON_WIDTH 66

PanelHomeButton::PanelHomeButton ()
: TextureArea (NUX_TRACKER_LOCATION),
  _util_cg (CAIRO_FORMAT_ARGB32, BUTTON_WIDTH, PANEL_HEIGHT)
{
  _pixbuf = gdk_pixbuf_new_from_file (PKGDATADIR"/bfb.png", NULL);
  SetMinMaxSize (BUTTON_WIDTH, PANEL_HEIGHT);

  OnMouseClick.connect (sigc::mem_fun (this, &PanelHomeButton::RecvMouseClick));
  
  // send this information over ubus
  OnMouseEnter.connect (sigc::mem_fun(this, &PanelHomeButton::RecvMouseEnter));
  OnMouseLeave.connect (sigc::mem_fun(this, &PanelHomeButton::RecvMouseLeave));
  OnMouseMove.connect  (sigc::mem_fun(this, &PanelHomeButton::RecvMouseMove));

  Refresh ();
}

PanelHomeButton::~PanelHomeButton ()
{
  g_object_unref (_pixbuf);
}

void
PanelHomeButton::Refresh ()
{
  int width = BUTTON_WIDTH;
  int height = PANEL_HEIGHT;

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *cr = cairo_graphics.GetContext();
  cairo_set_line_width (cr, 1);

  gdk_cairo_set_source_pixbuf (cr, _pixbuf,
                               (BUTTON_WIDTH-gdk_pixbuf_get_width (_pixbuf))/2,
                               (PANEL_HEIGHT-gdk_pixbuf_get_height (_pixbuf))/2);
  cairo_paint (cr);

  cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 0.2f);
  cairo_rectangle (cr, width-2, 2, 1, height-4);
  cairo_fill (cr);

  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.1f);
  cairo_rectangle (cr, width-1, 2, 1, height-4);
  cairo_fill (cr);

  cairo_destroy (cr);

  nux::NBitmapData* bitmap =  cairo_graphics.GetBitmap();

  // The Texture is created with a reference count of 1.
  nux::BaseTexture* texture2D = nux::GetThreadGLDeviceFactory ()->CreateSystemCapableTexture ();
  texture2D->Update(bitmap);
  delete bitmap;

  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap (nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

  nux::ROPConfig rop;
  rop.Blend = true;                       // Enable the blending. By default rop.Blend is false.
  rop.SrcBlend = GL_ONE;                  // Set the source blend factor.
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;  // Set the destination blend factor.
  nux::TextureLayer* texture_layer = new nux::TextureLayer (texture2D->GetDeviceTexture(),
                                                            texxform,           // The Oject that defines the texture wraping and coordinate transformation.
                                                            nux::Color::White,  // The color used to modulate the texture.
                                                            true,  // Write the alpha value of the texture to the destination buffer.
                                                            rop     // Use the given raster operation to set the blending when the layer is being rendered.
                                                            );

  SetPaintLayer(texture_layer);

  // We don't need the texture anymore. Since it hasn't been reference, it ref count should still be 1.
  // UnReference it and it will be destroyed.
  texture2D->UnReference ();

  // The texture layer has been cloned by this object when calling SetPaintLayer. It is safe to delete it now.
  delete texture_layer;

  NeedRedraw ();
}

void
PanelHomeButton::RecvMouseClick (int x,
                                 int y,
                                 unsigned long button_flags,
                                 unsigned long key_flags)
{
  if (nux::GetEventButton (button_flags) == 1)
    {
      UBusServer *ubus = ubus_server_get_default ();
      ubus_server_send_message (ubus, UBUS_HOME_BUTTON_ACTIVATED, NULL);
    }
}

void 
PanelHomeButton::RecvMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  GVariantBuilder builder;
  
  g_variant_builder_init (&builder, G_VARIANT_TYPE ("(iia{sv})"));
  g_variant_builder_add (&builder, "i", x);
  g_variant_builder_add (&builder, "i", y);
  
  g_variant_builder_open (&builder, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_add (&builder, "{sv}", "hovered", g_variant_new_boolean (true));
  g_variant_builder_close (&builder);
  

  UBusServer *ubus = ubus_server_get_default ();
  ubus_server_send_message (ubus, UBUS_HOME_BUTTON_TRIGGER_UPDATE, g_variant_builder_end (&builder));
}

void 
PanelHomeButton::RecvMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  GVariantBuilder builder;
  
  g_variant_builder_init (&builder, G_VARIANT_TYPE ("(iia{sv})"));
  g_variant_builder_add (&builder, "i", x);
  g_variant_builder_add (&builder, "i", y);
  
  g_variant_builder_open (&builder, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_add (&builder, "{sv}", "hovered", g_variant_new_boolean (false));
  g_variant_builder_close (&builder);
  

  UBusServer *ubus = ubus_server_get_default ();
  ubus_server_send_message (ubus, UBUS_HOME_BUTTON_TRIGGER_UPDATE, g_variant_builder_end (&builder));
}

void 
PanelHomeButton::RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  GVariantBuilder builder;
  
  g_variant_builder_init (&builder, G_VARIANT_TYPE ("(iia{sv})"));
  g_variant_builder_add (&builder, "i", x);
  g_variant_builder_add (&builder, "i", y);
  
  g_variant_builder_open (&builder, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_close (&builder);
  

  UBusServer *ubus = ubus_server_get_default ();
  ubus_server_send_message (ubus, UBUS_HOME_BUTTON_TRIGGER_UPDATE, g_variant_builder_end (&builder));
}

const gchar*
PanelHomeButton::GetName ()
{
	return "HomeButton";
}

void
PanelHomeButton::AddProperties (GVariantBuilder *builder)
{
  nux::Geometry geo = GetGeometry ();

  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));
  g_variant_builder_add (builder, "{sv}", "have-pixbuf", g_variant_new_boolean (GDK_IS_PIXBUF (_pixbuf)));
}
