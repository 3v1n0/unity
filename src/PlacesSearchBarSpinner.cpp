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

#include <Nux/VLayout.h>

#include "PlacesStyle.h"

NUX_IMPLEMENT_OBJECT_TYPE (PlacesSearchBarSpinner);

PlacesSearchBarSpinner::PlacesSearchBarSpinner ()
: nux::View (NUX_TRACKER_LOCATION),
  _state (STATE_READY),
  _rotation (0.0f),
  _spinner_timeout (0)
{
  PlacesStyle *style = PlacesStyle::GetDefault ();

  _search_ready = style->GetSearchReadyIcon ();
  _clear_full = style->GetSearchClearIcon ();
  _clear_alone = style->GetSearchClearAloneIcon ();
  _clear_spinner = style->GetSearchClearSpinnerIcon ();

  _2d_rotate.Identity ();
  _2d_rotate.Rotate_z (0.0);
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
  texxform.SetWrap (nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.min_filter = nux::TEXFILTER_LINEAR;
  texxform.mag_filter = nux::TEXFILTER_LINEAR;

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
    nux::Geometry clear_geo (geo.x + ((geo.width - _clear_spinner->GetWidth ())/2),
                             geo.y + ((geo.height - _clear_spinner->GetHeight ())/2),
                             _clear_spinner->GetWidth (),
                             _clear_spinner->GetHeight ());

    GfxContext.PushModelViewMatrix (nux::Matrix4::TRANSLATE(-clear_geo.x - clear_geo.width / 2,
                                    -clear_geo.y - clear_geo.height / 2, 0));
    GfxContext.PushModelViewMatrix (_2d_rotate);    
    GfxContext.PushModelViewMatrix (nux::Matrix4::TRANSLATE(clear_geo.x + clear_geo.width/ 2,
                                    clear_geo.y + clear_geo.height / 2, 0));

    GfxContext.QRP_1Tex (clear_geo.x,
                         clear_geo.y,
                         clear_geo.width,
                         clear_geo.height,
                         _clear_spinner->GetDeviceTexture (),
                         texxform,
                         nux::Color::White);

    GfxContext.PopModelViewMatrix ();
    GfxContext.PopModelViewMatrix ();
    GfxContext.PopModelViewMatrix ();

    GfxContext.QRP_1Tex (geo.x + ((geo.width - _clear_alone->GetWidth ())/2),
                         geo.y + ((geo.height - _clear_alone->GetHeight ())/2),
                         _clear_alone->GetWidth (),
                         _clear_alone->GetHeight (),
                         _clear_alone->GetDeviceTexture (),
                         texxform,
                         nux::Color::White);
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

gboolean
PlacesSearchBarSpinner::OnFrame (PlacesSearchBarSpinner *self)
{
  self->_rotation += 0.1f;

  if (self->_rotation >= 360.0f)
    self->_rotation = 0.0f;

  self->_2d_rotate.Rotate_z (self->_rotation);

  self->QueueDraw ();

  return TRUE;
}

void
PlacesSearchBarSpinner::SetState (SpinnerState state)
{
  if (_state == state)
    return;

  _state = state;

  if (_spinner_timeout)
    g_source_remove (_spinner_timeout);
  _2d_rotate.Rotate_z (0.0f);
  _rotation = 0.0f;

  if (_state == STATE_SEARCHING)
  {
    _spinner_timeout = g_timeout_add (15, (GSourceFunc)PlacesSearchBarSpinner::OnFrame, this);
  }

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
