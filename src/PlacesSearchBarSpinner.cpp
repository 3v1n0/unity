// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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

#include "PlacesSearchBarSpinner.h"

#include "PlacesStyle.h"

NUX_IMPLEMENT_OBJECT_TYPE (PlacesSearchBarSpinner);

PlacesSearchBarSpinner::PlacesSearchBarSpinner ()
: nux::View (NUX_TRACKER_LOCATION),
  _state (STATE_READY)
{
  PlacesStyle *style = PlacesStyle::GetDefault ();

  _search_ready = style->GetSearchReadyIcon ();
  _clear_full = style->GetSearchClearIcon ();
}

PlacesSearchBarSpinner::~PlacesSearchBarSpinner ()
{

}

long
PlacesSearchBarSpinner::ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  return PostProcessEvent2 (ievent, TraverseInfo, ProcessEventInfo);
}

void
PlacesSearchBarSpinner::Draw (nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry ();
  nux::TexCoordXForm texxform;

  GfxContext.PushClippingRectangle (geo);

  nux::GetPainter ().PaintBackground (GfxContext, geo);

  texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap (nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

  if (_state == STATE_READY)
  {
    GfxContext.QRP_1Tex (geo.x + ((geo.width - _search_ready->GetWidth ())/2),
                         geo.y + ((geo.height - _search_ready->GetHeight ())/2),
                         _search_ready->GetWidth (),
                         _search_ready->GetHeight (),
                         _search_ready->GetDeviceTexture (),
                         texxform,
                         nux::Color::White);
  }
  else if (_state == STATE_SEARCHING)
  {

  }
  else
  {
    GfxContext.QRP_1Tex (geo.x + ((geo.width - _clear_full->GetWidth ())/2),
                         geo.y + ((geo.height - _clear_full->GetHeight ())/2),
                         _clear_full->GetWidth (),
                         _clear_full->GetHeight (),
                         _clear_full->GetDeviceTexture (),
                         texxform,
                         nux::Color::White);
  }

  GfxContext.PopClippingRectangle ();
}

void
PlacesSearchBarSpinner::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{

}

void
PlacesSearchBarSpinner::SetState (SpinnerState state)
{
  if (_state == state)
    return;

  _state = state;
  QueueDraw ();
}

const gchar*
PlacesSearchBarSpinner::GetName ()
{
	return "PlacesSearchBarSpinner";
}

void PlacesSearchBarSpinner::AddProperties (GVariantBuilder *builder)
{
  nux::Geometry geo = GetGeometry ();

  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));
}
