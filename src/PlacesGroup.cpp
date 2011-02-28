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

#include <sigc++/sigc++.h>

#include <Nux/Nux.h>
#include <Nux/VLayout.h>
#include <Nux/HLayout.h>
#include <Nux/BaseWindow.h>
#include <NuxCore/Math/MathInc.h>

#include "StaticCairoText.h"

#include "Introspectable.h"

#include <sigc++/trackable.h>
#include <sigc++/signal.h>
#include <sigc++/functors/ptr_fun.h>
#include <sigc++/functors/mem_fun.h>

#include "PlacesGroup.h"
#include <glib.h>
#include <glib/gi18n-lib.h>

PlacesGroup::PlacesGroup (NUX_FILE_LINE_DECL)
: View (NUX_FILE_LINE_PARAM),
  _idle_id (0),
  _title_string (NULL),
  _content (NULL)
{
  _icon_texture = new IconTexture ("", 24);
  _icon_texture->SetMinimumSize (24, 24);

  _label = new nux::StaticCairoText ("", NUX_TRACKER_LOCATION);
  _label->SetTextEllipsize (nux::StaticCairoText::NUX_ELLIPSIZE_END);
  _label->SetTextAlignment (nux::StaticCairoText::NUX_ALIGN_LEFT);

  _title = new nux::StaticCairoText ("", NUX_TRACKER_LOCATION);
  _title->SetTextEllipsize (nux::StaticCairoText::NUX_ELLIPSIZE_END);
  _title->SetTextAlignment (nux::StaticCairoText::NUX_ALIGN_LEFT);

  _header_layout = new nux::HLayout (NUX_TRACKER_LOCATION);
  _header_layout->SetHorizontalInternalMargin (12);

  _header_layout->AddView (_icon_texture, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);
  _header_layout->AddView (_title, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);
  _header_layout->AddSpace (1, 1);
  _header_layout->AddView (_label, 0, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_FULL);

  _group_layout = new nux::VLayout ("", NUX_TRACKER_LOCATION);
  _group_layout->AddLayout (_header_layout, 0, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_FULL);

  SetLayout (_group_layout);
}

PlacesGroup::~PlacesGroup ()
{
  g_free (_title_string);
}

void PlacesGroup::SetTitle (const char *title)
{
  const gchar *temp = "<big>%s</big>";
  gchar *tmp = g_markup_escape_text (title, -1);

  _title_string = g_strdup_printf (temp, tmp);
  UpdateTitle ();

  g_free (tmp);
}

void PlacesGroup::SetEmblem (const char *path_to_emblem)
{
  _icon_texture->SetByIconName (path_to_emblem, 24);
}

void PlacesGroup::SetChildLayout (nux::Layout *layout)
{
  _content = layout;

  // By setting the stretch factor of the GridHLayout to 0, the height of the grid
  // will be forced to the height that is necessary to include all its elements.
  _group_layout->AddLayout (_content, 1);
  QueueDraw ();
}

nux::Layout *
PlacesGroup::GetChildLayout ()
{
  return _content;
}

void
PlacesGroup::UpdateTitle ()
{
  _title->SetText (_title_string);
  ComputeChildLayout ();
  QueueDraw ();
}

void
PlacesGroup::UpdateLabel ()
{
#if 0
  char *result_string = NULL;
  result_string = g_strdup_printf (g_dngettext(NULL, "See %s less results",
                                                     "See one less result",
                                                       _total_items - _visible_items));

  _label->SetText (result_string);

  ComputeChildLayout ();
  QueueDraw ();
  
  g_free ((result_string));
#endif
}

void PlacesGroup::Relayout ()
{
  if (_idle_id == 0)
    _idle_id = g_idle_add ((GSourceFunc)OnIdleRelayout, this);
}

gboolean PlacesGroup::OnIdleRelayout (PlacesGroup *self)
{
  self->QueueDraw ();
  self->_group_layout->QueueDraw ();
  self->GetChildLayout ()->QueueDraw ();
  self->ComputeChildLayout ();
  self->_idle_id = 0;

  return FALSE;
}

long PlacesGroup::ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;

  if (_group_layout)
    ret = _group_layout->ProcessEvent (ievent, TraverseInfo, ProcessEventInfo);

  return ret;
}

void PlacesGroup::Draw (nux::GraphicsEngine& GfxContext,
                       bool                 forceDraw)
{
}

void
PlacesGroup::DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw)
{
  nux::Geometry base = GetGeometry ();
  GfxContext.PushClippingRectangle (base);

  _group_layout->ProcessDraw (GfxContext, force_draw || IsFullRedraw ());

  GfxContext.PopClippingRectangle();
}

void
PlacesGroup::PostDraw (nux::GraphicsEngine &GfxContext, bool force_draw)
{

}
