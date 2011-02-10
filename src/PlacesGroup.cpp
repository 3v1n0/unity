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

PlacesGroup::PlacesGroup (NUX_FILE_LINE_DECL) :
View (NUX_FILE_LINE_PARAM)
{
  //~ OnMouseDown.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseDown));
  //~ OnMouseUp.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseUp));
  //~ OnMouseClick.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseClick));
  //~ OnMouseMove.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseMove));
  //~ OnMouseEnter.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseEnter));
  //~ OnMouseLeave.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseLeave));

  _label = new nux::StaticCairoText ("", NUX_TRACKER_LOCATION);
  _label->SetFont ("Ubuntu normal 11");
  _label->SetTextEllipsize (nux::StaticCairoText::NUX_ELLIPSIZE_END);
  _label->SetTextAlignment (nux::StaticCairoText::NUX_ALIGN_LEFT);
  _label->SetMaximumWidth (320);
  _label->SetMinimumWidth (1);

  _title = new nux::StaticCairoText ("", NUX_TRACKER_LOCATION);
  _title->SetFont ("Ubuntu normal 11");
  _title->SetTextEllipsize (nux::StaticCairoText::NUX_ELLIPSIZE_END);
  _title->SetTextAlignment (nux::StaticCairoText::NUX_ALIGN_RIGHT);
  _title->SetMaximumWidth (320);
  _title->SetMinimumWidth (1);

  _header_layout = new nux::HLayout ("", NUX_TRACKER_LOCATION);

  _header_layout->AddView (_title, 0, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_FULL);
  _header_layout->AddSpace (1, 1);

  // FIXME: We don't want to show this as it does nothing right now
  // _header_layout->AddView (_label, 0, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_FULL);

  _group_layout = new nux::VLayout ("", NUX_TRACKER_LOCATION);

  _group_layout->AddLayout (_header_layout, 0, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_FULL);

  _content = NULL;
  _expanded = false;
  _title_string = NULL;
  _row_height = 0;
  _total_items = 0;
  _visible_items = 0;

  SetCompositionLayout (_group_layout);

}

PlacesGroup::~PlacesGroup ()
{
  g_free (_title_string);

  _group_layout->RemoveChildObject (_header_layout);

  if (_content != NULL)
  {
    _group_layout->RemoveChildObject (_content);
  }
}

void PlacesGroup::SetTitle (const char *title)
{
  _title_string = g_strdup (title);
  UpdateTitle ();
}

void PlacesGroup::SetEmblem (const char *path_to_emblem)
{
}

void PlacesGroup::AddLayout (nux::Layout *layout)
{
  _content = layout;

  // By setting the stretch factor of the GridHLayout to 0, the height of the grid
  // will be forced to the height that is necessary to include all its elements.
  _group_layout->AddLayout (_content, 0);
  NeedRedraw ();
}

nux::Layout *
PlacesGroup::GetLayout ()
{
  return _content;
}

void PlacesGroup::SetRowHeight (unsigned int row_height)
{
  _row_height = row_height;
}

void PlacesGroup::SetItemDetail (unsigned int total_items, unsigned int visible_items)
{
  _total_items = total_items;
  _visible_items = visible_items;
  UpdateLabel ();
}

void PlacesGroup::SetExpanded (bool expanded)
{
  if (_expanded == expanded)
    return;

  _expanded = expanded;
  UpdateLabel ();
}

void
PlacesGroup::UpdateTitle ()
{
  _title->SetText (_title_string);
  ComputeChildLayout ();
  NeedRedraw ();
}

void
PlacesGroup::UpdateLabel ()
{
  if (_expanded)
  {
    _label->SetText (_("See less results"));
  }
  else
  {
    char *result_string = NULL;
    result_string = g_strdup_printf (g_dngettext(NULL, "See %s less results",
                                                       "See one less result",
                                                       _total_items - _visible_items));

    _label->SetText (result_string);
    g_free ((result_string));
  }

  ComputeChildLayout ();
  NeedRedraw ();
}

void
PlacesGroup::SetVisible (bool visible)
{
  _is_visible = visible;
  ComputeChildLayout ();
  NeedRedraw ();
}

bool
PlacesGroup::IsVisible ()
{
  return _is_visible;
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

  _group_layout->ProcessDraw (GfxContext, force_draw);

  GfxContext.PopClippingRectangle();
}

void
PlacesGroup::PostDraw (nux::GraphicsEngine &GfxContext, bool force_draw)
{

}

void
PlacesGroup::PreLayoutManagement ()
{
  nux::View::PreLayoutManagement ();
}

long
PlacesGroup::PostLayoutManagement (long LayoutResult)
{
  return nux::View::PostLayoutManagement (LayoutResult);
}

