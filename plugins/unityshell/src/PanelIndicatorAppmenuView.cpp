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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <UnityCore/Variant.h>
#include <UnityCore/GLibWrapper.h>
#include <libbamf/libbamf.h>

#include "PanelIndicatorAppmenuView.h"
#include "WindowManager.h"
#include "PanelStyle.h"

namespace unity
{
namespace
{
  const std::string MENU_ATOM = "_UBUNTU_APPMENU_UNIQUE_NAME";
}

using indicator::Entry;

PanelIndicatorAppmenuView::PanelIndicatorAppmenuView(Entry::Ptr const& proxy)
  : PanelIndicatorEntryView(proxy, 0, APPMENU)
  , xid_(0)
  , has_menu_(false)
{
  spacing_ = 2;
  right_padding_ = 7; 
}

void PanelIndicatorAppmenuView::Activate()
{
  ShowMenu(1);
}

void PanelIndicatorAppmenuView::ShowMenu(int button)
{
  if (xid_ && has_menu_)
  {
    WindowManager::Default()->Activate(xid_);

    proxy_->ShowMenu(xid_,
                     GetAbsoluteX(),
                     GetAbsoluteY() + panel::Style::Instance().panel_height,
                     1,
                     time(nullptr));
  }
}

std::string PanelIndicatorAppmenuView::GetLabel()
{
  glib::String escaped(g_markup_escape_text(label_.c_str(), -1));

  std::ostringstream bold_label;
  bold_label << "<b>" << escaped.Str() << "</b>";

  return bold_label.str();
}

bool PanelIndicatorAppmenuView::IsLabelSensitive() const
{
  return (!label_.empty() && has_menu_);
}

bool PanelIndicatorAppmenuView::IsLabelVisible() const
{
  return !label_.empty();
}

bool PanelIndicatorAppmenuView::IsIconSensitive() const
{
  return has_menu_;
}

bool PanelIndicatorAppmenuView::IsIconVisible() const
{
  return has_menu_;
}

void PanelIndicatorAppmenuView::SetLabel(std::string const& label)
{
  if (label_ != label)
  {
    label_ = label;
    Refresh();
  }
}

void PanelIndicatorAppmenuView::SetControlledWindowXid(Window xid)
{
  if (xid_ != xid)
  {
    xid_ = xid;
    CheckWindowMenu();
  }
}

bool PanelIndicatorAppmenuView::CheckWindowMenu()
{
  has_menu_ = false;

  if (!xid_)
    return false;

  glib::Object<BamfMatcher> matcher(bamf_matcher_get_default());

  GList* windows = bamf_matcher_get_windows(matcher);

  for (GList* l = windows; l; l = l->next)
  {
    if (BAMF_IS_WINDOW(l->data))
    {
      auto window = static_cast<BamfWindow*>(l->data);

      if (bamf_window_get_xid(window) == xid_)
      {
        glib::String property(bamf_window_get_utf8_prop(window, MENU_ATOM.c_str()));
        has_menu_ = bool(property);
        break;
      }
    }
  }

  g_list_free(windows);

  return has_menu_;
}

void PanelIndicatorAppmenuView::DrawEntryPrelight(cairo_t* cr, unsigned int width, unsigned int height)
{
  nux::Rect const& geo = proxy_->geometry();
  GtkStyleContext* style_context = panel::Style::Instance().GetStyleContext();
  int flair_width = std::min(geo.width, GetMinimumWidth());

  cairo_translate (cr, geo.x - GetAbsoluteX(), geo.y - height);

  gtk_style_context_save(style_context);

  GtkWidgetPath* widget_path = gtk_widget_path_new();
  gtk_widget_path_iter_set_name(widget_path, -1 , "UnityPanelWidget");
  gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_BAR);
  gtk_widget_path_append_type(widget_path, GTK_TYPE_MENU_ITEM);

  gtk_style_context_set_path(style_context, widget_path);
  gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUBAR);
  gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUITEM);
  gtk_style_context_set_state(style_context, GTK_STATE_FLAG_PRELIGHT);

  cairo_save(cr);
  cairo_push_group(cr);

  gtk_render_background(style_context, cr, 0, 0, flair_width, height);
  gtk_render_frame(style_context, cr, 0, 0, flair_width, height);

  cairo_pattern_t* pat = cairo_pop_group(cr);
  cairo_pattern_t* mask = cairo_pattern_create_linear(0, 0, 0, height);
  cairo_pattern_add_color_stop_rgba(mask, 0.0f, 0, 0, 0, 0.0f);
  cairo_pattern_add_color_stop_rgba(mask, 0.6f, 0, 0, 0, 0.0f);
  cairo_pattern_add_color_stop_rgba(mask, 1.0f, 0, 0, 0, 1.0f);

  cairo_rectangle(cr, 0, 0, flair_width, height);
  cairo_set_source(cr, pat);
  cairo_mask(cr, mask);

  cairo_pattern_destroy(pat);
  cairo_pattern_destroy(mask);
  cairo_restore(cr);

  gtk_widget_path_free(widget_path);

  gtk_style_context_restore(style_context);
}

std::string PanelIndicatorAppmenuView::GetName() const
{
  return "appmenu";
}

void PanelIndicatorAppmenuView::AddProperties(GVariantBuilder* builder)
{
  PanelIndicatorEntryView::AddProperties(builder);

  variant::BuilderWrapper(builder)
  .add("controlled-window", xid_)
  .add("has_menu", has_menu_);
}

} // NAMESPACE
