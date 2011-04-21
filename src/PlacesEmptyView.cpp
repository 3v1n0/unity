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

#include "PlacesEmptyView.h"

#include "PlacesStyle.h"

NUX_IMPLEMENT_OBJECT_TYPE (PlacesEmptyView);

PlacesEmptyView::PlacesEmptyView ()
: nux::View (NUX_TRACKER_LOCATION)
{
  PlacesStyle *style = PlacesStyle::GetDefault ();

  SetLayout (new nux::HLayout (NUX_TRACKER_LOCATION));

  _text = new nux::StaticCairoText ("");
  _text->SetMaximumWidth (style->GetTileWidth () * style->GetDefaultNColumns ());
  _text->SetTextEllipsize (nux::StaticCairoText::NUX_ELLIPSIZE_END);
  _text->SetTextAlignment (nux::StaticCairoText::NUX_ALIGN_CENTRE);
  _text->SetTextVerticalAlignment (nux::StaticCairoText::NUX_ALIGN_CENTRE);
  _text->SetTextColor (nux::Color (1.0f, 1.0f, 1.0f, 0.8f));

  GetCompositionLayout ()->AddSpace (1, 1);
  GetCompositionLayout ()->AddView (_text, 1, nux::eCenter, nux::eFix);
  GetCompositionLayout ()->AddSpace (1, 1);
}

PlacesEmptyView::~PlacesEmptyView ()
{

}

long
PlacesEmptyView::ProcessEvent(nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  return GetCompositionLayout ()->ProcessEvent (ievent, TraverseInfo, ProcessEventInfo);
}

void
PlacesEmptyView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
}

void
PlacesEmptyView::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  GetCompositionLayout ()->ProcessDraw (GfxContext, force_draw);
}

void
PlacesEmptyView::SetText (const char *text)
{
  char *clean;
  char *markup;

  clean = g_markup_escape_text (text, -1);
  markup = g_strdup_printf ("<big>%s</big>", clean);

  _text->SetText (markup);

  g_free (clean);
  g_free (markup);
}

const gchar*
PlacesEmptyView::GetName ()
{
  return "PlacesEmptyView";
}

void
PlacesEmptyView::AddProperties (GVariantBuilder *builder)
{
  
}
