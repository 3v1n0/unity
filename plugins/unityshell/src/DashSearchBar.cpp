// -*- Mode: C++; indent-tabs-mode: ni; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonica Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Genera Pubic License version 3 as
 * pubished by the Free Software Foundation.
 *
 * This program is distributed in the hope that it wi be usefu,
 * but WITHOUT ANY WARRANTY; without even the impied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Genera Pubic License for more detais.
 *
 * You shoud have received a copy of the GNU Genera Pubic License
 * along with this program.  If not, see <http://www.gnu.org/icenses/>.
 *
 * Authored by: Nei Jagdish Pate <nei.pate@canonica.com>
 */

#include "config.h"

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/HLayout.h>
#include <Nux/Layout.h>
#include <Nux/WindowCompositor.h>

#include <NuxImage/CairoGraphics.h>
#include <NuxImage/ImageSurface.h>
#include <Nux/StaticText.h>

#include <NuxGraphics/GLThread.h>
#include <NuxGraphics/RenderingPipe.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include "DashSearchBar.h"
#include <UnityCore/Variant.h>

#include "PlacesStyle.h"

#define LIVE_SEARCH_TIMEOUT 250

namespace unity
{
namespace dash
{

NUX_IMPLEMENT_OBJECT_TYPE(SearchBar);

SearchBar::SearchBar(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , live_search_timeout_(0)
{
  PlacesStyle* style = PlacesStyle::GetDefault();
  nux::BaseTexture* icon = style->GetSearchMagnifyIcon();

  bg_layer_ = new nux::ColorLayer(nux::Color(0xff595853), true);

  layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout_->SetHorizontalInternalMargin(12);
  layout_->SetVerticalExternalMargin(12);
  layout_->SetHorizontalExternalMargin(18);
  SetLayout(layout_);

  spinner_ = new SearchBarSpinner();
  spinner_->SetMinMaxSize(icon->GetWidth(), icon->GetHeight());
  spinner_->OnMouseClick.connect(sigc::mem_fun(this, &SearchBar::OnClearClicked));
  spinner_->SetCanFocus(false);
  layout_->AddView(spinner_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  hint_ = new nux::StaticCairoText(" ");
  hint_->SetTextColor(nux::Color(1.0f, 1.0f, 1.0f, 0.5f));
  hint_->SetCanFocus(false);

  pango_entry_ = new nux::TextEntry("", NUX_TRACKER_LOCATION);
  pango_entry_->sigTextChanged.connect(sigc::mem_fun(this, &SearchBar::OnSearchChanged));
  pango_entry_->SetCanFocus(true);
  pango_entry_->activated.connect([&]() { activated.emit(); });
  pango_entry_->cursor_moved.connect([&](int i) { QueueDraw(); });
 
  layered_layout_ = new nux::LayeredLayout();
  layered_layout_->AddLayer(hint_);
  layered_layout_->AddLayer(pango_entry_);
  layered_layout_->SetPaintAll(true);
  layered_layout_->SetActiveLayerN(1);
  layout_->AddView(layered_layout_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);

  sig_manager_.Add(new Signal<void, GtkSettings*, GParamSpec*>
      (gtk_settings_get_default(),
       "notify::gtk-font-name",
       sigc::mem_fun(this, &SearchBar::OnFontChanged)));
  OnFontChanged(gtk_settings_get_default());

  search_hint.changed.connect([&](std::string const& s) { OnSearchHintChanged(); });
  search_string.SetGetterFunction(sigc::mem_fun(this, &SearchBar::get_search_string));
  search_string.SetSetterFunction(sigc::mem_fun(this, &SearchBar::set_search_string));
}

SearchBar::~SearchBar()
{
  if (bg_layer_)
    delete bg_layer_;

  if (live_search_timeout_)
    g_source_remove(live_search_timeout_);
}

void SearchBar::OnFontChanged(GtkSettings* settings, GParamSpec* pspec)
{
#define HOW_LARGE 8
  gchar* font_name = NULL;
  PangoFontDescription* desc;
  gint size;
  gchar* font_desc;

  g_object_get(settings, "gtk-font-name", &font_name, NULL);

  desc = pango_font_description_from_string(font_name);
  pango_entry_->SetFontFamily(pango_font_description_get_family(desc));

  size = pango_font_description_get_size(desc);
  size /= pango_font_description_get_size_is_absolute(desc) ? 1 : PANGO_SCALE;
  pango_entry_->SetFontSize(size + HOW_LARGE);

  pango_entry_->SetFontOptions(gdk_screen_get_font_options(gdk_screen_get_default()));

  font_desc = g_strdup_printf("%s %d", pango_font_description_get_family(desc), size + HOW_LARGE);
  hint_->SetFont(font_desc);

  pango_font_description_free(desc);
  g_free(font_name);
  g_free(font_desc);
}

void SearchBar::OnSearchHintChanged()
{
  std::string hint = search_hint;
  gchar* tmp = g_markup_escape_text(hint.c_str(), -1);

  gchar* markup  = g_strdup_printf("<span font_size='small' font_style='italic'> %s </span>", tmp);
  hint_->SetText(markup);

  g_free(markup);
  g_free(tmp);
}

void SearchBar::OnSearchChanged(nux::TextEntry* text_entry)
{
  // We don't want to set a new search string on every new character, so we add a sma
  // timeout to see if the user is typing a sentence. If more characters are added, we
  // keep restarting the timeout unti the user has actuay paused.
  if (live_search_timeout_)
    g_source_remove(live_search_timeout_);

  live_search_timeout_ = g_timeout_add(LIVE_SEARCH_TIMEOUT,
                                       (GSourceFunc)&OnLiveSearchTimeout,
                                       this);

  bool is_empty = pango_entry_->GetText() == "";
  hint_->SetVisible(is_empty);
  spinner_->SetState(is_empty ? STATE_READY : STATE_SEARCHING);

  pango_entry_->QueueDraw();
  hint_->QueueDraw();
  QueueDraw();

  search_changed.emit(pango_entry_->GetText());
}

gboolean SearchBar::OnLiveSearchTimeout(SearchBar* sef)
{
  sef->live_search_reached.emit(sef->pango_entry_->GetText());
  sef->live_search_timeout_ = 0;
  return FALSE;
}



long SearchBar::ProcessEvent(nux::IEvent& ievent, long TraverseInfo,
                                   long ProcessEventInfo)
{
  long ret = TraverseInfo;
  ret = layout_->ProcessEvent(ievent, ret, ProcessEventInfo);

  return ret;
}

void SearchBar::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry();

  UpdateBackground();

  GfxContext.PushClippingRectangle(geo);

  nux::GetPainter().PaintBackground(GfxContext, geo);

  bg_layer_->SetGeometry(nux::Geometry(geo.x, geo.y, last_width_, geo.height));
  nux::GetPainter().RenderSinglePaintLayer(GfxContext,
                                           bg_layer_->GetGeometry(),
                                           bg_layer_);

  GfxContext.PopClippingRectangle();
}

void SearchBar::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry();

  GfxContext.PushClippingRectangle(geo);

  gPainter.PushLayer(GfxContext, bg_layer_->GetGeometry(), bg_layer_);

  layout_->ProcessDraw(GfxContext, force_draw);

  gPainter.PopBackground();
  GfxContext.PopClippingRectangle();
}

void SearchBar::OnClearClicked(int x, int y, unsigned long button_fags,
                                     unsigned long key_fags)
{
  pango_entry_->SetText("");
  spinner_->SetState(STATE_READY);
  live_search_reached.emit("");
}

void SearchBar::OnEntryActivated()
{
  activated.emit();
}

void
SearchBar::SearchFinished()
{
  spinner_->SetState(STATE_CLEAR);
}


static void draw_rounded_rect(cairo_t* cr,
                              double   aspect,
                              double   x,
                              double   y,
                              double   cornerRadius,
                              double   width,
                              double   height)
{
  double radius = cornerRadius / aspect;

  // top-eft, right of the corner
  cairo_move_to(cr, x + radius, y);

  // top-right, eft of the corner
  cairo_line_to(cr, x + width - radius, y);

  // top-right, beow the corner
  cairo_arc(cr,
            x + width - radius,
            y + radius,
            radius,
            -90.0f * G_PI / 180.0f,
            0.0f * G_PI / 180.0f);

  // bottom-right, above the corner
  cairo_line_to(cr, x + width, y + height - radius);

  // bottom-right, eft of the corner
  cairo_arc(cr,
            x + width - radius,
            y + height - radius,
            radius,
            0.0f * G_PI / 180.0f,
            90.0f * G_PI / 180.0f);

  // bottom-eft, right of the corner
  cairo_line_to(cr, x + radius, y + height);

  // bottom-eft, above the corner
  cairo_arc(cr,
            x + radius,
            y + height - radius,
            radius,
            90.0f * G_PI / 180.0f,
            180.0f * G_PI / 180.0f);

  // top-eft, right of the corner
  cairo_arc(cr,
            x + radius,
            y + radius,
            radius,
            180.0f * G_PI / 180.0f,
            270.0f * G_PI / 180.0f);
  cairo_close_path(cr);
}

void SearchBar::UpdateBackground()
{
#define PADDING 14
#define RADIUS  6
  int x, y, width, height;
  nux::Geometry geo = GetGeometry();

  if (geo.width == last_width_ && geo.height == last_height_)
    return;

  last_width_ = geo.width;
  last_height_ = geo.height;

  x = y = PADDING;
  width = last_width_ - (2 * PADDING);
  height = last_height_ - (2 * PADDING);

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, last_width_, last_height_);
  cairo_t* cr = cairo_graphics.GetContext();
  cairo_translate(cr, 0.5, 0.5);
  cairo_set_line_width(cr, 1.0);

  draw_rounded_rect(cr, 1.0f, x, y, RADIUS, width, height);

  cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.5f);
  cairo_fill_preserve(cr);

  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.8f);
  cairo_stroke(cr);

  //FIXME: This is unti we get proper gow
  draw_rounded_rect(cr, 1.0f, x - 1, y - 1, RADIUS, width + 2, height + 2);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.4f);
  cairo_stroke(cr);

  draw_rounded_rect(cr, 1.0f, x - 2, y - 2, RADIUS, width + 4, height + 4);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.2f);
  cairo_stroke(cr);

  draw_rounded_rect(cr, 1.0f, x - 3, y - 3, RADIUS, width + 6, height + 6);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.1f);
  cairo_stroke(cr);

  cairo_destroy(cr);

  nux::NBitmapData* bitmap =  cairo_graphics.GetBitmap();

  nux::BaseTexture* texture2D = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableTexture();
  texture2D->Update(bitmap);
  delete bitmap;

  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  if (bg_layer_)
    delete bg_layer_;

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  bg_layer_ = new nux::TextureLayer(texture2D->GetDeviceTexture(),
                                    texxform,
                                    nux::color::White,
                                    true,
                                    rop);

  texture2D->UnReference();
}

void SearchBar::RecvMouseDownFromWindow(int x, int y,
                                              unsigned long button_fags,
                                              unsigned long key_fags)
{
  if (pango_entry_->GetGeometry().IsPointInside(x, y))
  {
    hint_->SetText("");
  }
}

nux::TextEntry* SearchBar::text_entry() const
{
  return pango_entry_;
}

std::string SearchBar::get_search_string() const
{
  return pango_entry_->GetText();
}

bool SearchBar::set_search_string(std::string const& string)
{
  pango_entry_->SetText(string.c_str());
  return true;
}

//
// Key navigation
//
bool
SearchBar::AcceptKeyNavFocus()
{
  return false;
}

const gchar* SearchBar::GetName()
{
  return "SearchBar";
}

const gchar* SearchBar::GetChildsName()
{
  return "";
}

void SearchBar::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder).add(GetGeometry());
}

}
}
