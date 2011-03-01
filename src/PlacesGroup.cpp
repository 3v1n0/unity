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

#include <Nux/Utils.h>

#include "PlacesStyle.h"

static const nux::Color kExpandDefaultTextColor (1.0f, 1.0f, 1.0f, 0.6f);
static const nux::Color kExpandHoverTextColor (1.0f, 1.0f, 1.0f, 1.0f);


PlacesGroup::PlacesGroup (NUX_FILE_LINE_DECL)
: View (NUX_FILE_LINE_PARAM),
  _content_layout (NULL),
  _idle_id (0),
  _is_expanded (true),
  _n_visible_items_in_unexpand_mode (0),
  _n_total_items (0),
  _child_unexpand_height (0)
{
  PlacesStyle *style = PlacesStyle::GetDefault ();
  nux::BaseTexture *arrow = style->GetGroupUnexpandIcon ();

  _group_layout = new nux::VLayout ("", NUX_TRACKER_LOCATION);

  _header_layout = new nux::HLayout (NUX_TRACKER_LOCATION);
  _group_layout->AddLayout (_header_layout, 0, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_FULL);

  _icon = new IconTexture ("", 24);
  _icon->SetMinMaxSize (24, 24);
  _header_layout->AddView (_icon, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);

  _name = new nux::StaticCairoText ("", NUX_TRACKER_LOCATION);
  _name->SetTextEllipsize (nux::StaticCairoText::NUX_ELLIPSIZE_END);
  _name->SetTextAlignment (nux::StaticCairoText::NUX_ALIGN_LEFT);
  _header_layout->AddView (_name, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);

  _expand_label = new nux::StaticCairoText ("", NUX_TRACKER_LOCATION);
  _expand_label->SetTextEllipsize (nux::StaticCairoText::NUX_ELLIPSIZE_END);
  _expand_label->SetTextAlignment (nux::StaticCairoText::NUX_ALIGN_LEFT);
  _expand_label->SetTextColor (kExpandDefaultTextColor);
  _header_layout->AddView (_expand_label, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);

  _expand_icon = new nux::TextureArea ();
  _expand_icon->SetTexture (arrow);
  _expand_icon->SetMinimumSize (arrow->GetWidth (), arrow->GetHeight ());
  _header_layout->AddView (_expand_icon, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);

  SetLayout (_group_layout);

  _icon->OnMouseClick.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseClick));
  _icon->OnMouseEnter.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseEnter));
  _icon->OnMouseLeave.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseLeave));
  _name->OnMouseClick.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseClick));
  _name->OnMouseEnter.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseEnter));
  _name->OnMouseLeave.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseLeave));
  _expand_label->OnMouseClick.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseClick));
  _expand_label->OnMouseEnter.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseEnter));
  _expand_label->OnMouseLeave.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseLeave));
  _expand_icon->OnMouseClick.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseClick));
  _expand_icon->OnMouseEnter.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseEnter));
  _expand_icon->OnMouseLeave.connect (sigc::mem_fun (this, &PlacesGroup::RecvMouseLeave));
}

PlacesGroup::~PlacesGroup ()
{

}

void
PlacesGroup::SetName (const char *name)
{
  // Spaces are on purpose, want padding to be proportional to the size of the text
  // Bear with me, I'm trying something different :)
  const gchar *temp = "    <big>%s</big>    ";
  gchar *tmp, *final;

  tmp = g_markup_escape_text (name, -1);

  final = g_strdup_printf (temp, tmp);

  _name->SetText (final);

  g_free (tmp);
  g_free (final);
}

void
PlacesGroup::SetIcon (const char *path_to_emblem)
{
  _icon->SetByIconName (path_to_emblem, 24);
}

void
PlacesGroup::SetChildLayout (nux::Layout *layout)
{
  _content_layout = layout;

  // By setting the stretch factor of the GridHLayout to 0, the height of the grid
  // will be forced to the height that is necessary to include all its elements.
  //_group_layout->AddLayout (_content_layout, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  _group_layout->AddLayout (_content_layout, 1);
  QueueDraw ();
}

nux::Layout *
PlacesGroup::GetChildLayout ()
{
  return _content_layout;
}

void
PlacesGroup::Refresh ()
{
  const char *temp = "<small>%s</small>";
  char       *result_string;
  char       *final;

  if (_n_visible_items_in_unexpand_mode >= _n_total_items)
  {
    result_string = g_strdup ("");
  }
  else if (_is_expanded)
  {
    result_string = g_strdup (_("See fewer results"));
  }
  else
  {
    result_string = g_strdup_printf (g_dngettext (GETTEXT_PACKAGE,
                                           "See one more result",
                                           "See %d more results",
                                           _n_total_items - _n_visible_items_in_unexpand_mode),
                                     _n_total_items - _n_visible_items_in_unexpand_mode);
  }

  _expand_icon->SetVisible (!(_n_visible_items_in_unexpand_mode >= _n_total_items && _n_total_items != 0));

  final = g_strdup_printf (temp, result_string);

  _expand_label->SetText (final);
  _expand_label->SetVisible (_n_visible_items_in_unexpand_mode < _n_total_items);

  ComputeChildLayout ();
  QueueDraw ();

  g_free ((result_string));
  g_free (final);
}

void
PlacesGroup::Relayout ()
{
  if (_idle_id == 0)
    _idle_id = g_idle_add ((GSourceFunc)OnIdleRelayout, this);
}

gboolean
PlacesGroup::OnIdleRelayout (PlacesGroup *self)
{
  self->Refresh ();
  self->QueueDraw ();
  self->_group_layout->QueueDraw ();
  self->GetChildLayout ()->QueueDraw ();
  self->ComputeChildLayout ();
  self->_idle_id = 0;

  return FALSE;
}

long
PlacesGroup::ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;
  if (GetGeometry ().IsPointInside (ievent.e_x, ievent.e_y)
      || !_child_unexpand_height)
  {
    ret = _group_layout->ProcessEvent (ievent, TraverseInfo, ProcessEventInfo);
  }
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
PlacesGroup::SetCounts (guint n_visible_items_in_unexpand_mode, guint n_total_items)
{
  _n_visible_items_in_unexpand_mode = n_visible_items_in_unexpand_mode;
  _n_total_items = n_total_items;

  Relayout ();
}


void
PlacesGroup::SetChildUnexpandHeight (guint height)
{
  if (_child_unexpand_height == height)
    return;

  _child_unexpand_height = height;

  if (!_is_expanded)
  {
    _is_expanded = true;
    SetExpanded (false);
  }
}

bool
PlacesGroup::GetExpanded ()
{
  return _is_expanded;
}

void
PlacesGroup::SetExpanded (bool is_expanded)
{
  PlacesStyle *style = PlacesStyle::GetDefault ();

  if (_is_expanded == is_expanded)
    return;

  _is_expanded = is_expanded;

  Refresh ();

  if (_content_layout)
  {
    _content_layout->SetMaximumHeight (_is_expanded ? nux::AREA_MAX_HEIGHT : _child_unexpand_height);
    SetMaximumHeight (_is_expanded ? nux::AREA_MAX_HEIGHT
                                   : _header_layout->GetGeometry ().height + _child_unexpand_height);

    ComputeChildLayout ();
    _group_layout->ComputeChildLayout ();
    _content_layout->ComputeChildLayout ();
    _content_layout->QueueDraw ();
    _group_layout->QueueDraw ();
    QueueDraw ();
  }

  _expand_icon->SetTexture (_is_expanded ? style->GetGroupUnexpandIcon ()
                                         : style->GetGroupExpandIcon ());

  expanded.emit ();
}

void
PlacesGroup::RecvMouseClick (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  SetExpanded (!_is_expanded);
}

void
PlacesGroup::RecvMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _expand_label->SetTextColor (kExpandHoverTextColor);
}

void
PlacesGroup::RecvMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _expand_label->SetTextColor (kExpandDefaultTextColor);
}
