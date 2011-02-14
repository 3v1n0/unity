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

#include "Nux/Nux.h"
#include "Nux/HLayout.h"
#include "Nux/VLayout.h"

#include "NuxGraphics/GLThread.h"
#include "Nux/BaseWindow.h"
#include "Nux/WindowCompositor.h"

#include "PanelIndicatorObjectEntryView.h"
#include "PanelStyle.h"

#include <glib.h>
#include <pango/pangocairo.h>
#include <gtk/gtk.h>
#include <time.h>


static void draw_menu_bg (cairo_t *cr, int width, int height);


PanelIndicatorObjectEntryView::PanelIndicatorObjectEntryView (IndicatorObjectEntryProxy *proxy, int padding)
: TextureArea (NUX_TRACKER_LOCATION),
  _proxy (proxy),
  _util_cg (CAIRO_FORMAT_ARGB32, 1, 1)
{
  _proxy->active_changed.connect (sigc::mem_fun (this, &PanelIndicatorObjectEntryView::OnActiveChanged));
  _proxy->updated.connect (sigc::mem_fun (this, &PanelIndicatorObjectEntryView::Refresh));
  _padding = padding;

  InputArea::OnMouseDown.connect (sigc::mem_fun (this, &PanelIndicatorObjectEntryView::OnMouseDown));
  InputArea::OnMouseWheel.connect (sigc::mem_fun (this, &PanelIndicatorObjectEntryView::OnMouseWheel));
  
  Refresh ();
}

PanelIndicatorObjectEntryView::~PanelIndicatorObjectEntryView ()
{
}

void
PanelIndicatorObjectEntryView::OnActiveChanged (bool is_active)
{
  active_changed.emit (this, is_active);
}

void
PanelIndicatorObjectEntryView::OnMouseDown (int x, int y, long button_flags, long key_flags)
{
  if (_proxy->GetActive ())
    return;

  if ((_proxy->label_visible && _proxy->label_sensitive)
      || (_proxy->icon_visible && _proxy->icon_sensitive))
  {
    _proxy->ShowMenu (GetGeometry ().x + 1, //cairo translation
                      PANEL_HEIGHT,
                      time (NULL),
                      nux::GetEventButton (button_flags));
  }
}

void
PanelIndicatorObjectEntryView::OnMouseWheel (int x, int y, int delta, unsigned long mouse_state, unsigned long key_state)
{
  _proxy->Scroll (delta);
}

void
PanelIndicatorObjectEntryView::Activate ()
{
  _proxy->ShowMenu (GetGeometry ().x + 1, //cairo translation FIXME: Make this into one function
                    PANEL_HEIGHT,
                    time (NULL),
                    1);
}

static char *
fix_string (const char *string)
{
  if (string == NULL)
    return NULL;

  char buf[256];
  int buf_pos = 0;
  int i = 0;
  int len = strlen (string);

  for (i = 0; i < len; i++)
    {
      if (string[i] != '_')
        {
          buf[buf_pos] = string[i];
          buf_pos++;
        }
    }
  buf[buf_pos] = '\0';

  return g_strdup (buf);
}

// We need to do a couple of things here:
// 1. Figure out our width
// 2. Figure out if we're active
// 3. Paint something
void
PanelIndicatorObjectEntryView::Refresh ()
{
  GdkPixbuf            *pixbuf = _proxy->GetPixbuf ();
  char                 *label = fix_string (_proxy->GetLabel ());
  PangoLayout          *layout = NULL;
  PangoFontDescription *desc = NULL;
  GtkSettings          *settings = gtk_settings_get_default ();
  cairo_t              *cr;
  char                 *font_description = NULL;
  GdkScreen            *screen = gdk_screen_get_default ();
  int                   dpi = 0;

  int  x = 0;
  int  y = 0;
  int  width = 0;
  int  height = PANEL_HEIGHT;
  int  icon_width = 0;
  int  text_width = 0;
  int  text_height = 0;


  // First lets figure out our size
  if (pixbuf && _proxy->icon_visible)
  {
    width = gdk_pixbuf_get_width (pixbuf);
    icon_width = width;
  }

  if (label && _proxy->label_visible)
  {
    PangoContext *cxt;
    PangoRectangle log_rect;

    cr = _util_cg.GetContext ();

    g_object_get (settings,
                  "gtk-font-name", &font_description,
                  "gtk-xft-dpi", &dpi,
                  NULL);
    desc = pango_font_description_from_string (font_description);
    pango_font_description_set_weight (desc, PANGO_WEIGHT_NORMAL);

    layout = pango_cairo_create_layout (cr);
    pango_layout_set_font_description (layout, desc);
    pango_layout_set_text (layout, label, -1);
    
    cxt = pango_layout_get_context (layout);
    pango_cairo_context_set_font_options (cxt, gdk_screen_get_font_options (screen));
    pango_cairo_context_set_resolution (cxt, (float)dpi/(float)PANGO_SCALE);
    pango_layout_context_changed (layout);

    pango_layout_get_extents (layout, NULL, &log_rect);
    text_width = log_rect.width / PANGO_SCALE;
    text_height = log_rect.height / PANGO_SCALE;

    if (icon_width)
      width += SPACING;
    width += text_width;

    pango_font_description_free (desc);
    g_free (font_description);
    cairo_destroy (cr);
  }

  if (width)
    width += _padding *2;

  SetMinimumWidth (width);

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_graphics.GetContext();
  cairo_set_line_width (cr, 1);

  if (_proxy->GetActive ())
    draw_menu_bg (cr, width, height);

  x = _padding;
  y = 0;

  if (_proxy->GetPixbuf () && _proxy->icon_visible)
  {
    gdk_cairo_set_source_pixbuf (cr, pixbuf, x, (height - gdk_pixbuf_get_height (pixbuf))/2);
    cairo_paint_with_alpha (cr, _proxy->icon_sensitive ? 1.0 : 0.5);

    x += icon_width + SPACING;

    g_object_unref (pixbuf);
  }

  if (label && _proxy->label_visible)
  {
    pango_cairo_update_layout (cr, layout);

    // Once for the homies that couldn't be here
    cairo_set_source_rgb (cr, 50/255.0f, 50/255.0f, 45/255.0f);
    cairo_move_to (cr, x, ((height - text_height)/2)-1);
    pango_cairo_show_layout (cr, layout);
    cairo_stroke (cr);

    // Once again for the homies that could
    cairo_set_source_rgba (cr, 223/255.0f, 219/255.0f, 210/255.0f,
                           _proxy->label_sensitive ? 1.0f : 0.0f);
    cairo_move_to (cr, x, (height - text_height)/2);
    pango_cairo_show_layout (cr, layout);
    cairo_stroke (cr);
  }

  cairo_destroy (cr);
  if (layout)
    g_object_unref (layout);

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
                                                            false,  // Write the alpha value of the texture to the destination buffer.
                                                            rop     // Use the given raster operation to set the blending when the layer is being rendered.
                                                            );

  SetPaintLayer (texture_layer);
    
  // We don't need the texture anymore. Since it hasn't been reference, it ref count should still be 1.
  // UnReference it and it will be destroyed.
  texture2D->UnReference ();

  // The texture layer has been cloned by this object when calling SetPaintLayer. It is safe to delete it now.
  delete texture_layer;

  NeedRedraw ();

  refreshed.emit (this);
  if (label)
    g_free (label);
}

static void
draw_menu_bg (cairo_t *cr, int width, int height)
{
  int radius = 4;
  double x = 0;
  double y = 0;
  double xos = 0.5;
  double yos = 0.5;
  /* FIXME */
  double mpi = 3.14159265358979323846;

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  cairo_set_line_width (cr, 1.0);

  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.2);

  cairo_move_to (cr, x+xos+radius, y+yos);
  cairo_arc (cr, x+xos+width-xos*2-radius, y+yos+radius, radius, mpi*1.5, mpi*2);
  cairo_line_to (cr, x+xos+width-xos*2, y+yos+height-yos*2+2);
  cairo_line_to (cr, x+xos, y+yos+height-yos*2+2);
  cairo_arc (cr, x+xos+radius, y+yos+radius, radius, mpi, mpi*1.5);

  cairo_pattern_t * pat = cairo_pattern_create_linear (x+xos, y, x+xos, y+height-yos*2+2);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 83/255.0f, 82/255.0f, 78/255.0f, 1.0f);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 66/255.0f, 65/255.0f, 63/255.0f, 1.0f);
  cairo_set_source (cr, pat);
  cairo_fill_preserve (cr);
  cairo_pattern_destroy (pat);

  pat = cairo_pattern_create_linear (x+xos, y, x+xos, y+height-yos*2+2);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 62/255.0f, 61/255.0f, 58/255.0f, 1.0f);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 54/255.0f, 54/255.0f, 52/255.0f, 1.0f);
  cairo_set_source (cr, pat);
  cairo_stroke (cr);
  cairo_pattern_destroy (pat);

  xos++;
  yos++;

  /* enlarging the area to not draw the lightborder at bottom, ugly trick :P */
  cairo_move_to (cr, x+radius+xos, y+yos);
  cairo_arc (cr, x+xos+width-xos*2-radius, y+yos+radius, radius, mpi*1.5, mpi*2);
  cairo_line_to (cr, x+xos+width-xos*2, y+yos+height-yos*2+3);
  cairo_line_to (cr, x+xos, y+yos+height-yos*2+3);
  cairo_arc (cr, x+xos+radius, y+yos+radius, radius, mpi, mpi*1.5);

  pat = cairo_pattern_create_linear (x+xos, y, x+xos, y+height-yos*2+3);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 92/255.0f, 90/255.0f, 85/255.0f, 1.0f);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 70/255.0f, 69/255.0f, 66/255.0f, 1.0f);
  cairo_set_source (cr, pat);
  cairo_stroke (cr);
  cairo_pattern_destroy (pat);
}

const gchar *
PanelIndicatorObjectEntryView::GetName ()
{
  const gchar *name = _proxy->GetId ();

  if (g_strcmp0 (name, "|") == 0)
    return NULL;
  else
   return name;
}

void
PanelIndicatorObjectEntryView::AddProperties (GVariantBuilder *builder)
{
  nux::Geometry geo = GetGeometry ();

  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));

  g_variant_builder_add (builder, "{sv}", "label", g_variant_new_string (_proxy->GetLabel ()));
  g_variant_builder_add (builder, "{sv}", "label_sensitive", g_variant_new_boolean (_proxy->label_sensitive));
  g_variant_builder_add (builder, "{sv}", "label_visible", g_variant_new_boolean (_proxy->label_visible));

  g_variant_builder_add (builder, "{sv}", "icon_sensitive", g_variant_new_boolean (_proxy->icon_sensitive));
  g_variant_builder_add (builder, "{sv}", "icon_visible", g_variant_new_boolean (_proxy->icon_visible));

  g_variant_builder_add (builder, "{sv}", "active", g_variant_new_boolean (_proxy->GetActive ()));
}
