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

#include "config.h"
#include "TextInput.h"

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include <Nux/LayeredLayout.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <NuxCore/Logger.h>

#include "CairoTexture.h"
#include "StaticCairoText.h"
#include "IconTexture.h"
#include "IMTextEntry.h"
#include "DashStyle.h"
#include "PreviewStyle.h"
#include "RawPixel.h"
#include "TextureCache.h"
#include "ThemeSettings.h"
#include "UnitySettings.h"

namespace unity
{

namespace
{
const int BORDER_RADIUS = 5;
const int TOOLTIP_WAIT = 500;
const RawPixel SPACE_BETWEEN_ENTRY_AND_HIGHLIGHT = 10_em;
const RawPixel LEFT_INTERNAL_PADDING = 6_em;
const RawPixel TEXT_INPUT_RIGHT_BORDER = 10_em;
const RawPixel HINT_PADDING = 3_em;

const RawPixel TOOLTIP_Y_OFFSET  =  3_em;
const RawPixel TOOLTIP_OFFSET    = 10_em;
const RawPixel DEFAULT_ICON_SIZE = 22_em;

std::string ACTIVATOR_ICON  = "arrow_right";
std::string WARNING_ICON    = "dialog-warning-symbolic";
// Fonts
const std::string HINT_LABEL_DEFAULT_FONT_NAME = "Ubuntu";
const int HINT_LABEL_FONT_SIZE = 11;

const std::string PANGO_ENTRY_DEFAULT_FONT_FAMILY = "Ubuntu";
const RawPixel PANGO_ENTRY_FONT_SIZE = 14_em;

nux::logging::Logger logger("unity.textinput");

std::shared_ptr<nux::AbstractPaintLayer> CreateWarningLayer(nux::BaseTexture* texture)
{
  // Create the texture layer
  nux::TexCoordXForm texxform;

  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.min_filter = nux::TEXFILTER_LINEAR;
  texxform.mag_filter = nux::TEXFILTER_LINEAR;

  nux::ROPConfig rop;
  rop.Blend = true;

  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  return std::make_shared<nux::TextureLayer>(texture->GetDeviceTexture(), texxform, nux::color::White, true, rop);
}
}

NUX_IMPLEMENT_OBJECT_TYPE(TextInput);

TextInput::TextInput(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , activator_icon(ACTIVATOR_ICON)
  , activator_icon_size(DEFAULT_ICON_SIZE)
  , background_color(nux::Color(0.0f, 0.0f, 0.0f, 0.35f))
  , border_color(nux::Color(1.0f, 1.0f, 1.0f, 0.7f))
  , border_radius(BORDER_RADIUS)
  , input_hint("")
  , hint_font_name(HINT_LABEL_DEFAULT_FONT_NAME)
  , hint_font_size(HINT_LABEL_FONT_SIZE)
  , hint_color(nux::Color(1.0f, 1.0f, 1.0f, 0.5f))
  , show_activator(false)
  , show_lock_warnings(false)
  , scale(1.0)
  , bg_layer_(new nux::ColorLayer(nux::Color(0xff595853), true))
  , caps_lock_on(false)
  , last_width_(-1)
  , last_height_(-1)
{
  layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout_->SetLeftAndRightPadding(LEFT_INTERNAL_PADDING.CP(scale), TEXT_INPUT_RIGHT_BORDER.CP(scale));
  layout_->SetSpaceBetweenChildren(SPACE_BETWEEN_ENTRY_AND_HIGHLIGHT.CP(scale));
  SetLayout(layout_);

  hint_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  hint_layout_->SetLeftAndRightPadding(HINT_PADDING.CP(scale), HINT_PADDING.CP(scale));

  hint_ = new StaticCairoText("");
  hint_->SetTextColor(hint_color);
  hint_color.changed.connect(sigc::hide(sigc::mem_fun(this, &TextInput::UpdateHintColor)));

  hint_->SetScale(scale);
  hint_layout_->AddView(hint_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  hint_font_name.changed.connect(sigc::hide(sigc::mem_fun(this, &TextInput::UpdateHintFont)));
  hint_font_size.changed.connect(sigc::hide(sigc::mem_fun(this, &TextInput::UpdateHintFont)));
  UpdateHintFont();

  pango_entry_ = new IMTextEntry();
  pango_entry_->SetFontFamily(PANGO_ENTRY_DEFAULT_FONT_FAMILY.c_str());
  pango_entry_->cursor_moved.connect([this](int i) { QueueDraw(); });
  pango_entry_->mouse_down.connect(sigc::mem_fun(this, &TextInput::OnMouseButtonDown));
  pango_entry_->end_key_focus.connect(sigc::mem_fun(this, &TextInput::OnEndKeyFocus));
  pango_entry_->text_changed.connect([this](nux::TextEntry*) {
    hint_->SetVisible(input_string().empty());
  });

  layered_layout_ = new nux::LayeredLayout();
  layered_layout_->AddLayer(hint_layout_);
  layered_layout_->AddLayer(pango_entry_);
  layered_layout_->SetPaintAll(true);
  layered_layout_->SetActiveLayerN(1);
  layout_->AddView(layered_layout_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);

  UpdateFont();

  // Caps lock warning
  warning_ = new IconTexture(LoadWarningIcon(DEFAULT_ICON_SIZE.CP(scale)));
  warning_->SetVisible(caps_lock_on());
  layout_->AddView(warning_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  caps_lock_on.changed.connect(sigc::mem_fun(this, &TextInput::OnLockStateChanged));

  show_lock_warnings.changed.connect(sigc::hide(sigc::mem_fun(this, &TextInput::CheckLocks)));
  scale.changed.connect(sigc::mem_fun(this, &TextInput::UpdateScale));
  Settings::Instance().font_scaling.changed.connect(sigc::hide(sigc::mem_fun(this, &TextInput::UpdateSize)));

  // Activator
  activator_ = new IconTexture(LoadActivatorIcon(activator_icon(), activator_icon_size().CP(scale)));
  activator_->SetVisible(show_activator());
  layout_->AddView(activator_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  activator_icon.changed.connect([this] (std::string icon) {
    activator_->SetTexture(LoadActivatorIcon(icon, activator_icon_size().CP(scale)));
  });

  activator_icon_size.changed.connect([this] (RawPixel icon_size) {
    activator_->SetTexture(LoadActivatorIcon(activator_icon(), icon_size.CP(scale)));
  });

  show_activator.changed.connect([this] (bool value) {
    activator_->SetVisible(value);
  });

  activator_->mouse_click.connect([this](int, int, unsigned long, unsigned long) {
    pango_entry_->activated.emit();
  });

  // Spinner
  spinner_ = new SearchBarSpinner();
  spinner_->SetVisible(false);
  spinner_->scale = scale();
  layout_->AddView(spinner_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  TextureCache::GetDefault().themed_invalidated.connect(sigc::mem_fun(this, &TextInput::UpdateTextures));
  theme::Settings::Get()->font.changed.connect(sigc::hide(sigc::mem_fun(this, &TextInput::UpdateFont)));
  sig_manager_.Add<void, GdkKeymap*>(gdk_keymap_get_default(), "state-changed", [this](GdkKeymap*) { CheckLocks(); });

  input_string.SetGetterFunction(sigc::mem_fun(this, &TextInput::get_input_string));
  input_string.SetSetterFunction(sigc::mem_fun(this, &TextInput::set_input_string));
  im_active.SetGetterFunction(sigc::mem_fun(this, &TextInput::get_im_active));
  im_preedit.SetGetterFunction(sigc::mem_fun(this, &TextInput::get_im_preedit));
  input_hint.changed.connect([this](std::string const& s) { OnInputHintChanged(); });

  warning_->mouse_enter.connect([this] (int x, int y, int button, int key_flags) {
    tooltip_timeout_.reset(new glib::Timeout(TOOLTIP_WAIT, [this] {
      tooltip_timeout_.reset();
      QueueDraw();
      return false;
    }));
  });

  warning_->mouse_leave.connect([this] (int x, int y, int button, int key_flags) {
    tooltip_timeout_ ? tooltip_timeout_.reset() : QueueDraw();
  });

  //background
  background_color.changed.connect([this] (nux::Color color) { UpdateBackground(true); });
  border_color.changed.connect([this] (nux::Color color) { UpdateBackground(true); });
  border_radius.changed.connect([this] (int radius) { UpdateBackground(true); });
}

void TextInput::UpdateSize()
{
  pango_entry_->SetFontSize(PANGO_ENTRY_FONT_SIZE.CP(scale * Settings::Instance().font_scaling()));
  int entry_min = pango_entry_->GetMinimumHeight();
  pango_entry_->SetMaximumHeight(entry_min);
  layered_layout_->SetMinimumHeight(entry_min);
  layered_layout_->SetMaximumHeight(entry_min);
}

void TextInput::UpdateScale(double scale)
{
  layout_->SetLeftAndRightPadding(LEFT_INTERNAL_PADDING.CP(scale), TEXT_INPUT_RIGHT_BORDER.CP(scale));
  layout_->SetSpaceBetweenChildren(SPACE_BETWEEN_ENTRY_AND_HIGHLIGHT.CP(scale));

  UpdateSize();

  hint_layout_->SetLeftAndRightPadding(HINT_PADDING.CP(scale), HINT_PADDING.CP(scale));
  hint_->SetScale(scale);
  hint_->SetMaximumHeight(pango_entry_->GetMinimumHeight());

  spinner_->scale = scale;
  activator_->SetTexture(LoadActivatorIcon(activator_icon(), activator_icon_size().CP(scale)));
  warning_->SetTexture(LoadWarningIcon(DEFAULT_ICON_SIZE.CP(scale)));
  warning_tooltip_.Release();

  QueueRelayout();
  QueueDraw();
}

void TextInput::UpdateTextures()
{
  activator_->SetTexture(LoadActivatorIcon(activator_icon(), activator_icon_size().CP(scale)));
  QueueDraw();
}

void TextInput::CheckLocks()
{
  GdkKeymap* keymap = gdk_keymap_get_default();
  caps_lock_on = gdk_keymap_get_caps_lock_state(keymap) ? true : false;
}

void TextInput::OnLockStateChanged(bool)
{
  if (!show_lock_warnings)
  {
    warning_->SetVisible(false);
    return;
  }

  warning_->SetVisible(caps_lock_on());
  warning_->SetOpacity(1.0);
  warning_tooltip_.Release();
  QueueRelayout();
  QueueDraw();
}

void TextInput::SetSpinnerVisible(bool visible)
{
  spinner_->SetVisible(visible);
  activator_->SetVisible(!visible && show_activator());
}

void TextInput::SetSpinnerState(SpinnerState spinner_state)
{
  spinner_->SetState(spinner_state);
}

void TextInput::UpdateHintFont()
{
  hint_->SetFont((hint_font_name() + " " + std::to_string(hint_font_size())).c_str());
}

void TextInput::UpdateHintColor()
{
  hint_->SetTextColor(hint_color);
}

nux::ObjectPtr<nux::BaseTexture> TextInput::LoadActivatorIcon(std::string const& icon_file, int icon_size)
{
  TextureCache& cache = TextureCache::GetDefault();
  return cache.FindTexture(icon_file, icon_size, icon_size);
}

nux::ObjectPtr<nux::BaseTexture> TextInput::LoadWarningIcon(int icon_size)
{
  auto* theme = gtk_icon_theme_get_default();
  GtkIconLookupFlags flags = GTK_ICON_LOOKUP_FORCE_SIZE;
  glib::Error error;
  glib::Object<GdkPixbuf> pixbuf(gtk_icon_theme_load_icon(theme, WARNING_ICON.c_str(), icon_size, flags, &error));

  if (pixbuf != nullptr)
  {
    nux::CairoGraphics cg(CAIRO_FORMAT_ARGB32, gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));
    cairo_t* cr = cg.GetInternalContext();

    cairo_push_group(cr);
    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
    cairo_paint_with_alpha(cr, 1.0);
    std::shared_ptr<cairo_pattern_t> pat(cairo_pop_group(cr), cairo_pattern_destroy);

    cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
    cairo_rectangle(cr, 0, 0, gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));
    cairo_mask(cr, pat.get());

    return texture_ptr_from_cairo_graphics(cg);
  }
  // Use fallback icon (this one is a png, and does not scale!)
  else
  {
    dash::previews::Style& preview_style = dash::previews::Style::Instance();
    return nux::ObjectPtr<nux::BaseTexture>(preview_style.GetWarningIcon());
  }
}

void TextInput::LoadWarningTooltip()
{
  glib::Object<GtkStyleContext> style_context(gtk_style_context_new());
  std::shared_ptr<GtkWidgetPath> widget_path(gtk_widget_path_new(), gtk_widget_path_free);
  gtk_widget_path_append_type(widget_path.get(), GTK_TYPE_TOOLTIP);

  gtk_style_context_set_path(style_context, widget_path.get());
  gtk_style_context_add_class(style_context, "tooltip");

  glib::Object<PangoContext> context(gdk_pango_context_get());
  glib::Object<PangoLayout> layout(pango_layout_new(context));

  auto const& font = theme::Settings::Get()->font();
  std::shared_ptr<PangoFontDescription> desc(pango_font_description_from_string(font.c_str()), pango_font_description_free);
  pango_context_set_font_description(context, desc.get());
  pango_context_set_language(context, gtk_get_default_language());
  pango_cairo_context_set_resolution(context, 96.0 * Settings::Instance().font_scaling());

  pango_layout_set_height(layout, -1); //avoid wrap lines

  if (caps_lock_on())
  {
    pango_layout_set_text(layout, _("Caps lock is on"), -1);
  }

  nux::Size extents;
  pango_layout_get_pixel_size(layout, &extents.width, &extents.height);
  extents.width  += TOOLTIP_OFFSET;
  extents.height += TOOLTIP_OFFSET;

  nux::CairoGraphics cg(CAIRO_FORMAT_ARGB32, RawPixel(extents.width).CP(scale), RawPixel(extents.height).CP(scale));
  cairo_surface_set_device_scale(cg.GetSurface(), scale, scale);
  cairo_t* cr = cg.GetInternalContext();

  gtk_render_background(style_context, cr, 0, 0, extents.width, extents.height);
  gtk_render_frame(style_context, cr, 0, 0, extents.width, extents.height);
  gtk_render_layout(style_context, cr, TOOLTIP_OFFSET/2, TOOLTIP_OFFSET/2, layout);

  warning_tooltip_ = texture_ptr_from_cairo_graphics(cg);
}

void TextInput::UpdateFont()
{
  auto* desc = pango_font_description_from_string(theme::Settings::Get()->font().c_str());

  if (desc)
  {
    pango_entry_->SetFontFamily(pango_font_description_get_family(desc));
    pango_entry_->SetFontOptions(gdk_screen_get_font_options(gdk_screen_get_default()));
    UpdateSize();

    if (hint_font_name() == HINT_LABEL_DEFAULT_FONT_NAME)
    {
      std::ostringstream font_desc;
      font_desc << pango_font_description_get_family(desc) << " " << hint_font_size();
      hint_->SetFont(font_desc.str().c_str());
    }

    pango_font_description_free(desc);
  }
}

void TextInput::OnInputHintChanged()
{
  hint_->SetText(input_hint().c_str(), true);
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

  if (warning_->IsVisible() && warning_->IsMouseInside() && !tooltip_timeout_)
    PaintWarningTooltip(GfxContext);

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

void TextInput::PaintWarningTooltip(nux::GraphicsEngine& graphics_engine)
{
  nux::Geometry const& warning_geo = warning_->GetGeometry();

  if (!warning_tooltip_.IsValid())
    LoadWarningTooltip();

  nux::Geometry tooltip_geo = {warning_geo.x - (warning_tooltip_->GetWidth() + TOOLTIP_OFFSET.CP(scale) / 2),
                               warning_geo.y - TOOLTIP_Y_OFFSET.CP(scale),
                               warning_tooltip_->GetWidth(),
                               warning_tooltip_->GetHeight()};

  auto const& warning_layer = CreateWarningLayer(warning_tooltip_.GetPointer());
  nux::GetPainter().PushDrawLayer(graphics_engine, tooltip_geo, warning_layer.get());
}

void TextInput::UpdateBackground(bool force)
{
  nux::Geometry geo(GetGeometry());

  if (geo.width == last_width_ && geo.height == last_height_ && !force)
    return;

  last_width_ = geo.width;
  last_height_ = geo.height;

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, last_width_, last_height_);
  cairo_surface_set_device_scale(cairo_graphics.GetSurface(), scale, scale);
  cairo_t* cr = cairo_graphics.GetInternalContext();

  cairo_graphics.DrawRoundedRectangle(cr,
                                      1.0f,
                                      0.5, 0.5,
                                      border_radius(),
                                      (last_width_/scale) - 1, (last_height_/scale) - 1,
                                      false);

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba(cr, background_color().red, background_color().green, background_color().blue, background_color().alpha);
  cairo_fill_preserve(cr);
  cairo_set_line_width(cr, 1);
  cairo_set_source_rgba(cr, border_color().red, border_color().green, border_color().blue, border_color().alpha);
  cairo_stroke(cr);

  auto texture2D = texture_ptr_from_cairo_graphics(cairo_graphics);

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
}

void TextInput::OnMouseButtonDown(int x, int y, unsigned long button, unsigned long key)
{
  hint_->SetVisible(false);
}

void TextInput::OnEndKeyFocus()
{
  hint_->SetVisible(input_string().empty());
}


IMTextEntry* TextInput::text_entry() const
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
