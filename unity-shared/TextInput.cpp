// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more detais.
 *
 * You shoud have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Manuel de la Pena <manuel.delapena@canonical.com>
 */

#include "TextInput.h"

namespace
{
const float kExpandDefaultIconOpacity = 1.0f;

const int SPACE_BETWEEN_ENTRY_AND_HIGHLIGHT = 10;
const int LEFT_INTERNAL_PADDING = 6;
const int TEXT_INPUT_RIGHT_BORDER = 10;

const int HIGHLIGHT_HEIGHT = 24;

// Fonts
const std::string HINT_LABEL_FONT_SIZE = "12px";
const std::string HINT_LABEL_FONT_STYLE = "Italic";
const std::string HINT_LABEL_DEFAULT_FONT = "Ubuntu " + HINT_LABEL_FONT_STYLE + " " + HINT_LABEL_FONT_SIZE;

const std::string PANGO_ENTRY_DEFAULT_FONT_FAMILY = "Ubuntu";
const int PANGO_ENTRY_FONT_SIZE = 14;

}

namespace unity
{

nux::logging::Logger logger("unity.dash.textinput");

NUX_IMPLEMENT_OBJECT_TYPE(TextInput);

TextInput::TextInput(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , input_hint("")
  , last_width_(-1)
  , last_height_(-1)
{
  Init();
}

void TextInput::Init()
{
  bg_layer_.reset(new nux::ColorLayer(nux::Color(0xff595853), true));

  layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout_->SetLeftAndRightPadding(LEFT_INTERNAL_PADDING, TEXT_INPUT_RIGHT_BORDER);
  layout_->SetSpaceBetweenChildren(SPACE_BETWEEN_ENTRY_AND_HIGHLIGHT);
  SetLayout(layout_);

  nux::HLayout* hint_layout = new nux::HLayout(NUX_TRACKER_LOCATION);

  hint_ = new StaticCairoText(" ");
  hint_->SetTextColor(nux::Color(1.0f, 1.0f, 1.0f, 0.5f));
  hint_->SetFont(HINT_LABEL_DEFAULT_FONT.c_str());
  hint_layout->AddView(hint_,  0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  pango_entry_ = new IMTextEntry();
  pango_entry_->SetFontFamily(PANGO_ENTRY_DEFAULT_FONT_FAMILY.c_str());
  pango_entry_->SetFontSize(PANGO_ENTRY_FONT_SIZE);
  pango_entry_->cursor_moved.connect([this](int i) { QueueDraw(); });
  pango_entry_->mouse_down.connect(sigc::mem_fun(this, &TextInput::OnMouseButtonDown));
  pango_entry_->end_key_focus.connect(sigc::mem_fun(this, &TextInput::OnEndKeyFocus));

  layered_layout_ = new nux::LayeredLayout();
  layered_layout_->AddLayout(hint_layout);
  layered_layout_->AddLayer(pango_entry_);
  layered_layout_->SetPaintAll(true);
  layered_layout_->SetActiveLayerN(1);
  layout_->AddView(layered_layout_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);

  sig_manager_.Add<void, GtkSettings*, GParamSpec*>(gtk_settings_get_default(),
    "notify::gtk-font-name", sigc::mem_fun(this, &TextInput::OnFontChanged));
  OnFontChanged(gtk_settings_get_default());

  input_string.SetGetterFunction(sigc::mem_fun(this, &TextInput::get_input_string));
  input_string.SetSetterFunction(sigc::mem_fun(this, &TextInput::set_input_string));
  im_active.SetGetterFunction(sigc::mem_fun(this, &TextInput::get_im_active));
  im_preedit.SetGetterFunction(sigc::mem_fun(this, &TextInput::get_im_preedit));
  input_hint.changed.connect([this](std::string const& s) { OnInputHintChanged(); });

}

void TextInput::OnFontChanged(GtkSettings* settings, GParamSpec* pspec)
{
  glib::String font_name;
  PangoFontDescription* desc;
  std::ostringstream font_desc;

  g_object_get(settings, "gtk-font-name", &font_name, NULL);

  desc = pango_font_description_from_string(font_name.Value());
  if (desc)
  {
    pango_entry_->SetFontFamily(pango_font_description_get_family(desc));
    pango_entry_->SetFontSize(PANGO_ENTRY_FONT_SIZE);
    pango_entry_->SetFontOptions(gdk_screen_get_font_options(gdk_screen_get_default()));

    font_desc << pango_font_description_get_family(desc) << " " << HINT_LABEL_FONT_STYLE << " " << HINT_LABEL_FONT_SIZE;
    hint_->SetFont(font_desc.str().c_str());

    font_desc.str("");
    font_desc.clear();

    pango_font_description_free(desc);
  }
}

void TextInput::OnInputHintChanged()
{
  glib::String tmp(g_markup_escape_text(input_hint().c_str(), -1));
  hint_->SetText(tmp);
}

void TextInput::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  UpdateBackground(false);

  GfxContext.PushClippingRectangle(base);
  nux::GetPainter().PaintBackground(GfxContext, base);

  bg_layer_->SetGeometry(nux::Geometry(base.x, base.y, last_width_, last_height_));
  nux::GetPainter().RenderSinglePaintLayer(GfxContext,
                                           bg_layer_->GetGeometry(),
                                           bg_layer_.get());

  GfxContext.PopClippingRectangle();
}

void TextInput::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();

  GfxContext.PushClippingRectangle(geo);

  if (highlight_layer_ && ShouldBeHighlighted() && !IsFullRedraw())
  {
    nux::GetPainter().PushLayer(GfxContext, highlight_layer_->GetGeometry(), highlight_layer_.get());
  }


  if (!IsFullRedraw())
  {
    gPainter.PushLayer(GfxContext, bg_layer_->GetGeometry(), bg_layer_.get());
  }
  else
  {
    nux::GetPainter().PushPaintLayerStack();
  }

  layout_->ProcessDraw(GfxContext, force_draw);

  if (!IsFullRedraw())
  {
    gPainter.PopBackground();
  }
  else
  {
    nux::GetPainter().PopPaintLayerStack();
  }

  GfxContext.PopClippingRectangle();
}

void TextInput::UpdateBackground(bool force)
{
  int RADIUS = 5;
  nux::Geometry geo(GetGeometry());
  geo.width = layered_layout_->GetAbsoluteX() +
              layered_layout_->GetAbsoluteWidth() -
              GetAbsoluteX() +
              TEXT_INPUT_RIGHT_BORDER;

  LOG_DEBUG(logger) << "height: "
  << geo.height << " - "
  << layered_layout_->GetGeometry().height << " - "
  << pango_entry_->GetGeometry().height;

  if (geo.width == last_width_
      && geo.height == last_height_
      && force == false)
    return;

  last_width_ = geo.width;
  last_height_ = geo.height;

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, last_width_, last_height_);
  cairo_t* cr = cairo_graphics.GetContext();

  cairo_graphics.DrawRoundedRectangle(cr,
                                      1.0f,
                                      0.5, 0.5,
                                      RADIUS,
                                      last_width_ - 1, last_height_ - 1,
                                      false);

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.35f);
  cairo_fill_preserve(cr);
  cairo_set_line_width(cr, 1);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.7f);
  cairo_stroke(cr);

  cairo_destroy(cr);
  nux::BaseTexture* texture2D = texture_from_cairo_graphics(cairo_graphics);

  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  bg_layer_.reset(new nux::TextureLayer(texture2D->GetDeviceTexture(),
                                        texxform,
                                        nux::color::White,
                                        true,
                                        rop));

  texture2D->UnReference();
}

void TextInput::OnMouseButtonDown(int x, int y, unsigned long button, unsigned long key)
{
  hint_->SetVisible(false);
}

void TextInput::OnEndKeyFocus()
{
  hint_->SetVisible(input_string().empty());
}


nux::TextEntry* TextInput::text_entry() const
{
  return pango_entry_;
}

std::string TextInput::get_input_string() const
{
  return pango_entry_->GetText();
}

bool TextInput::set_input_string(std::string const& string)
{
  pango_entry_->SetText(string.c_str());
  return true;
}

bool TextInput::get_im_active() const
{
  return pango_entry_->im_active();
}

bool TextInput::get_im_preedit() const
{
  return pango_entry_->im_preedit();
}

//
// Highlight
//
bool TextInput::ShouldBeHighlighted()
{
  return true;
}

//
// Key navigation
//
bool TextInput::AcceptKeyNavFocus()
{
  return false;
}

//
// Introspection
//
std::string TextInput::GetName() const
{
  return "TextInput";
}

void TextInput::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
  .add(GetAbsoluteGeometry())
  .add("has_focus", pango_entry_->HasKeyFocus())
  .add("input_string", pango_entry_->GetText())
  .add("im_active", pango_entry_->im_active());
}

} // namespace unity
