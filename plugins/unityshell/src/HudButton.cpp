/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */
#include "config.h"

#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>
#include <NuxImage/CairoGraphics.h>
#include <NuxGraphics/NuxGraphics.h>
#include <UnityCore/GLibWrapper.h>
#include "HudButton.h"

namespace
{
nux::logging::Logger logger("unity.hud.HudButton");
}
// Create a texture from the CairoGraphics object.
//
// Returns a new BaseTexture that has a ref count of 1.
static nux::BaseTexture* texture_from_cairo_graphics(nux::CairoGraphics& cg)
{
  nux::NBitmapData* bitmap = cg.GetBitmap();
  nux::BaseTexture* tex = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableTexture();
  tex->Update(bitmap);
  delete bitmap;
  return tex;
}


namespace unity {
namespace hud {
  HudButton::HudButton (nux::TextureArea *image, NUX_FILE_LINE_DECL)
      : nux::Button (image, NUX_FILE_LINE_PARAM)
      , is_focused_(false)
  {
    RedrawTheme();
    OnKeyNavFocusChange.connect([this](nux::Area *area){ QueueDraw(); });
    label.changed.connect([this](std::string new_label) { SetLabel(new_label); RedrawTheme(); });
    hint.changed.connect([this](std::string new_label) { RedrawTheme(); });
  }

  HudButton::HudButton (const std::string label_, NUX_FILE_LINE_DECL)
      : nux::Button (NUX_FILE_LINE_PARAM)
      , is_focused_(false)
  {
    RedrawTheme();
    label.changed.connect([this](std::string new_label) { SetLabel(new_label); RedrawTheme(); });
    hint.changed.connect([this](std::string new_label) { RedrawTheme(); });
  }

  HudButton::HudButton (const std::string label_, nux::TextureArea *image, NUX_FILE_LINE_DECL)
      : nux::Button (image, NUX_FILE_LINE_PARAM)
      , is_focused_(false)
  {
    RedrawTheme();
    label.changed.connect([this](std::string new_label) { SetLabel(new_label); RedrawTheme(); });
    hint.changed.connect([this](std::string new_label) { RedrawTheme(); });
  }

  HudButton::HudButton (NUX_FILE_LINE_DECL)
      : nux::Button (NUX_FILE_LINE_PARAM)
      , is_focused_(false)
  {
    RedrawTheme();
    label.changed.connect([this](std::string new_label) { SetLabel(new_label); RedrawTheme(); });
  }

  HudButton::~HudButton() {
    normal_texture_->UnReference();
    focused_texture_->UnReference();
  }

  void HudButton::RedrawTheme ()
  {

    if (normal_texture_ != nullptr)
      normal_texture_->UnReference();

    if (focused_texture_ != nullptr)
      focused_texture_->UnReference();

    nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, GetGeometry().width, GetGeometry().height);
    cairo_t* cr = cairo_graphics.GetContext();

    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);

    int padding = 4;

    // draw the lines
    double width = GetGeometry().width;
    double height = GetGeometry().height;

    nux::Color line_color(1.0f, 1.0f, 1.0f, 0.5f);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_graphics.DrawLine(0.0, 0.0, 0.0, height, 1.0, line_color);
    cairo_graphics.DrawLine(0.0, height, width, height, 1.0, line_color);
    cairo_graphics.DrawLine(width, height, width, 0.0, 1.0, line_color);


    // draw the text
    PangoLayout*          layout     = NULL;
    PangoFontDescription* desc       = NULL;
    PangoContext*         pango_context   = NULL;
    PangoRectangle        logRect  = {0, 0, 0, 0};
    GdkScreen*            screen     = gdk_screen_get_default();    // not ref'ed
    glib::String          font;
    int                   dpi = -1;

    g_object_get(gtk_settings_get_default(), "gtk-font-name", &font, NULL);
    g_object_get(gtk_settings_get_default(), "gtk-xft-dpi", &dpi, NULL);

    cairo_set_font_options(cr, gdk_screen_get_font_options(screen));
    layout = pango_cairo_create_layout(cr);
    desc = pango_font_description_from_string(font.Value());
    pango_font_description_set_size (desc, 14 * PANGO_SCALE);

    pango_layout_set_font_description(layout, desc);
    pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);

    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    pango_layout_set_width(layout, (width - (padding * 2)) * PANGO_SCALE);
    pango_layout_set_height(layout, -1);

    pango_context = pango_layout_get_context(layout);  // is not ref'ed
    pango_cairo_context_set_font_options(pango_context,
                                         gdk_screen_get_font_options(screen));
    pango_cairo_context_set_resolution(pango_context,
                                       dpi == -1 ? 96.0f : dpi/(float) PANGO_SCALE);


    // Draw the first text item
    char *escaped_text = g_markup_escape_text(label().c_str()  , -1);
    pango_layout_set_markup(layout, escaped_text, -1);

    g_free (escaped_text);

    pango_layout_context_changed(layout);

    pango_layout_get_extents(layout, NULL, &logRect);
    int extent_height = (logRect.y + logRect.height) / PANGO_SCALE;

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.5f);

    cairo_move_to(cr, padding, ((height - extent_height) * 0.5));
    pango_cairo_show_layout(cr, layout);


    // Draw the second text item
    pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
    escaped_text = g_markup_escape_text(hint().c_str()  , -1);
    std::string string_final_text = "<small>" + std::string(escaped_text) + "</small>";
    pango_layout_set_markup(layout, string_final_text.c_str(), -1);

    g_free (escaped_text);

    pango_layout_context_changed(layout);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.5f);

    cairo_move_to(cr, 0.0f, 0.0f);
    pango_cairo_show_layout(cr, layout);

    normal_texture_ = texture_from_cairo_graphics(cairo_graphics);

    cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OVER);
    cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.5f);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    focused_texture_ = texture_from_cairo_graphics(cairo_graphics);

    // clean up
    pango_font_description_free(desc);
    g_object_unref(layout);
    cairo_destroy(cr);

    QueueDraw();
  }

  bool HudButton::AcceptKeyNavFocus()
  {
    return true;
  }


  long HudButton::ComputeContentSize ()
  {
    long ret = nux::Button::ComputeContentSize();
    if (cached_geometry_.width != GetGeometry().height || cached_geometry_.height != GetGeometry().height)
    {
      RedrawTheme();
    }

    cached_geometry_ = GetGeometry();
    return ret;
  }

  void HudButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    gPainter.PaintBackground(GfxContext, GetGeometry());
    // set up our texture mode
    nux::TexCoordXForm texxform;
    texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

    // clear what is behind us
    nux::t_u32 alpha = 0, src = 0, dest = 0;
    GfxContext.GetRenderStates().GetBlend(alpha, src, dest);
    GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    nux::Color col = nux::color::Black;
    col.alpha = 0;
    GfxContext.QRP_Color(GetGeometry().x,
                         GetGeometry().y,
                         GetGeometry().width,
                         GetGeometry().height,
                         col);

    BaseTexturePtr texture = normal_texture_;
    if (nux::GetWindowCompositor().GetKeyFocusArea() == this)
    {
      texture = focused_texture_;
    }


    GfxContext.QRP_1Tex(GetGeometry().x,
                        GetGeometry().y,
                        GetGeometry().width,
                        GetGeometry().height,
                        texture->GetDeviceTexture(),
                        texxform,
                        nux::Color(1.0f, 1.0f, 1.0f, 1.0f));

    GfxContext.GetRenderStates().SetBlend(alpha, src, dest);
  }

  void HudButton::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
  }

  void HudButton::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Button::PostDraw(GfxContext, force_draw);
  }

  void HudButton::SetSuggestion(Hud::Suggestion suggestion)
  {
    suggestion_ = suggestion;
    label = std::get<0>(suggestion);
  }

  Hud::Suggestion HudButton::GetSuggestion()
  {
    return suggestion_;
  }
}
}
