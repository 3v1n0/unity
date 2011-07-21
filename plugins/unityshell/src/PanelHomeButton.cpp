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

#include "PanelStyle.h"
#include <UnityCore/Variant.h>

#define PANEL_HEIGHT 24

NUX_IMPLEMENT_OBJECT_TYPE(PanelHomeButton);

PanelHomeButton::PanelHomeButton()
  : TextureArea(NUX_TRACKER_LOCATION),
    _opacity(1.0f),
    _urgent_interest(0)
{
  _urgent_count = 0;
  _button_width = 66;
  _pressed = false;
  SetMinMaxSize(_button_width, PANEL_HEIGHT);

  OnMouseClick.connect(sigc::mem_fun(this, &PanelHomeButton::RecvMouseClick));

  // send this information over ubus
  OnMouseEnter.connect(sigc::mem_fun(this, &PanelHomeButton::RecvMouseEnter));
  OnMouseLeave.connect(sigc::mem_fun(this, &PanelHomeButton::RecvMouseLeave));
  OnMouseMove.connect(sigc::mem_fun(this, &PanelHomeButton::RecvMouseMove));

  _theme_changed_id = g_signal_connect(gtk_icon_theme_get_default(), "changed",
                                       G_CALLBACK(PanelHomeButton::OnIconThemeChanged), this);

  _urgent_interest = ubus_server_register_interest(ubus_server_get_default(),
                                                   UBUS_LAUNCHER_ICON_URGENT_CHANGED,
                                                   (UBusCallback) &PanelHomeButton::OnLauncherIconUrgentChanged,
                                                   this);

  _shown_interest = ubus_server_register_interest(ubus_server_get_default(),
                                                  UBUS_PLACE_VIEW_SHOWN,
                                                  (UBusCallback)&PanelHomeButton::OnPlaceShown,
                                                  this);

  _hidden_interest = ubus_server_register_interest(ubus_server_get_default(),
                                                   UBUS_PLACE_VIEW_HIDDEN,
                                                   (UBusCallback)&PanelHomeButton::OnPlaceHidden,
                                                   this);

  Refresh();

  SetDndEnabled(false, true);
}

PanelHomeButton::~PanelHomeButton()
{
  if (_theme_changed_id)
    g_signal_handler_disconnect(gtk_icon_theme_get_default(), _theme_changed_id);

  if (_urgent_interest != 0)
    ubus_server_unregister_interest(ubus_server_get_default(),
                                    _urgent_interest);

  if (_shown_interest != 0)
    ubus_server_unregister_interest(ubus_server_get_default(),
                                    _shown_interest);

  if (_hidden_interest != 0)
    ubus_server_unregister_interest(ubus_server_get_default(),
                                    _hidden_interest);
}

void
PanelHomeButton::OnLauncherIconUrgentChanged(GVariant* data, gpointer user_data)
{
  PanelHomeButton* self = static_cast<PanelHomeButton*>(user_data);

  if (g_variant_get_boolean(data))
    self->_urgent_count++;
  else
    self->_urgent_count--;

  if (self->_urgent_count < 0)
    self->_urgent_count = 0;

  self->Refresh();
}

void
PanelHomeButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry();

  GfxContext.PushClippingRectangle(geo);

  nux::GetPainter().PaintBackground(GfxContext, geo);

  nux::TextureArea::Draw(GfxContext, force_draw);

  GfxContext.PopClippingRectangle();
}

void
PanelHomeButton::Refresh()
{
  int width = _button_width;
  int height = PANEL_HEIGHT;
  GdkPixbuf* pixbuf;

  SetMinMaxSize(_button_width, PANEL_HEIGHT);

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t* cr = cairo_graphics.GetContext();
  cairo_set_line_width(cr, 1);

  /* button pressed effect */
  //FIXME(Cimi) removed the pressed effect

  pixbuf = PanelStyle::GetDefault()->GetHomeButton();
  if (GDK_IS_PIXBUF(pixbuf))
  {
    int offset_x = 0;
    int offset_y = 0;

    /* if the button pressed, draw the icon 1 px to the right bottom */
    if (_pressed)
    {
      offset_x = offset_y = 1;
    }
    gdk_cairo_set_source_pixbuf(cr, pixbuf,
                                (_button_width - gdk_pixbuf_get_width(pixbuf)) / 2 + offset_x,
                                (PANEL_HEIGHT - gdk_pixbuf_get_height(pixbuf)) / 2 + offset_y);
    cairo_paint(cr);
    g_object_unref(pixbuf);
  }

  cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.2f);
  cairo_rectangle(cr, width - 2, 2, 1, height - 4);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.1f);
  cairo_rectangle(cr, width - 1, 2, 1, height - 4);
  cairo_fill(cr);

  if (_urgent_count)
  {
    cairo_set_source_rgba(cr, 0.18f, 0.8f, 0.95f, 1.0f);
    cairo_move_to(cr, 0, 0);
    cairo_line_to(cr, 8, 0);
    cairo_line_to(cr, 0, 8);
    cairo_line_to(cr, 0, 0);
    cairo_fill(cr);
  }

  cairo_destroy(cr);

  nux::NBitmapData* bitmap =  cairo_graphics.GetBitmap();

  // The Texture is created with a reference count of 1.
  nux::BaseTexture* texture2D = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableTexture();
  texture2D->Update(bitmap);
  delete bitmap;

  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

  nux::ROPConfig rop;
  rop.Blend = true;                       // Enable the blending. By default rop.Blend is false.
  rop.SrcBlend = GL_ONE;                  // Set the source blend factor.
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;  // Set the destination blend factor.
  nux::TextureLayer* texture_layer = new nux::TextureLayer(
    texture2D->GetDeviceTexture(),
    texxform, // The Oject that defines the texture wraping and coordinate transformation.
    nux::color::White,  // The color used to modulate the texture.
    true,  // Write the alpha value of the texture to the destination buffer.
    rop);  // Use the given raster operation to set the blending when the layer is being rendered.

  SetPaintLayer(texture_layer);

  // We don't need the texture anymore. Since it hasn't been reference, it ref count should still be 1.
  // UnReference it and it will be destroyed.
  texture2D->UnReference();

  // The texture layer has been cloned by this object when calling SetPaintLayer. It is safe to delete it now.
  delete texture_layer;

  NeedRedraw();
}

void
PanelHomeButton::RecvMouseClick(int x,
                                int y,
                                unsigned long button_flags,
                                unsigned long key_flags)
{
  if (nux::GetEventButton(button_flags) == 1)
  {
    UBusServer* ubus = ubus_server_get_default();
    ubus_server_send_message(ubus, UBUS_DASH_EXTERNAL_ACTIVATION, NULL);
  }
}

void
PanelHomeButton::RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  GVariantBuilder builder;

  g_variant_builder_init(&builder, G_VARIANT_TYPE("(iiiia{sv})"));
  g_variant_builder_add(&builder, "i", x);
  g_variant_builder_add(&builder, "i", y);
  g_variant_builder_add(&builder, "i", _button_width);
  g_variant_builder_add(&builder, "i", PANEL_HEIGHT);

  g_variant_builder_open(&builder, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&builder, "{sv}", "hovered", g_variant_new_boolean(true));
  g_variant_builder_close(&builder);


  UBusServer* ubus = ubus_server_get_default();
  ubus_server_send_message(ubus, UBUS_HOME_BUTTON_BFB_UPDATE, g_variant_builder_end(&builder));
}

void
PanelHomeButton::RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  GVariantBuilder builder;

  g_variant_builder_init(&builder, G_VARIANT_TYPE("(iiiia{sv})"));
  g_variant_builder_add(&builder, "i", x);
  g_variant_builder_add(&builder, "i", y);
  g_variant_builder_add(&builder, "i", _button_width);
  g_variant_builder_add(&builder, "i", PANEL_HEIGHT);

  g_variant_builder_open(&builder, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&builder, "{sv}", "hovered", g_variant_new_boolean(false));
  g_variant_builder_close(&builder);


  UBusServer* ubus = ubus_server_get_default();
  ubus_server_send_message(ubus, UBUS_HOME_BUTTON_BFB_UPDATE, g_variant_builder_end(&builder));
}

void
PanelHomeButton::RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  GVariantBuilder builder;

  g_variant_builder_init(&builder, G_VARIANT_TYPE("(iiiia{sv})"));
  g_variant_builder_add(&builder, "i", x);
  g_variant_builder_add(&builder, "i", y);
  g_variant_builder_add(&builder, "i", _button_width);
  g_variant_builder_add(&builder, "i", PANEL_HEIGHT);

  g_variant_builder_open(&builder, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&builder, "{sv}", "hovered", g_variant_new_boolean(true));
  g_variant_builder_close(&builder);


  UBusServer* ubus = ubus_server_get_default();
  ubus_server_send_message(ubus, UBUS_HOME_BUTTON_BFB_UPDATE, g_variant_builder_end(&builder));
}

void
PanelHomeButton::SetButtonWidth(int button_width)
{
  if (_button_width ==  button_width)
    return;

  _button_width =  button_width;

  Refresh();
}

const gchar*
PanelHomeButton::GetName()
{
  return "HomeButton";
}

void
PanelHomeButton::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder)
  .add(GetGeometry());
}

void
PanelHomeButton::OnIconThemeChanged(GtkIconTheme* icon_theme, gpointer data)
{
  PanelHomeButton* self = (PanelHomeButton*) data;

  self->Refresh();
}

void
PanelHomeButton::OnPlaceShown(GVariant* data, gpointer user_data)
{
  PanelHomeButton* self = static_cast<PanelHomeButton*>(user_data);

  self->_pressed = true;
  self->Refresh();
}

void
PanelHomeButton::OnPlaceHidden(GVariant* data, gpointer user_data)
{
  PanelHomeButton* self = static_cast<PanelHomeButton*>(user_data);

  self->_pressed = false;
  self->Refresh();
}

void
PanelHomeButton::ProcessDndEnter()
{
  UBusServer* ubus = ubus_server_get_default();
  ubus_server_send_message(ubus, UBUS_HOME_BUTTON_BFB_DND_ENTER, NULL);
}

void
PanelHomeButton::ProcessDndLeave()
{
}

void
PanelHomeButton::ProcessDndMove(int x, int y, std::list<char*> mimes)
{
  SendDndStatus(false, nux::DNDACTION_NONE, GetAbsoluteGeometry());
}

void
PanelHomeButton::ProcessDndDrop(int x, int y)
{
}

void
PanelHomeButton::SetOpacity(float opacity)
{
  _opacity = opacity;
}
