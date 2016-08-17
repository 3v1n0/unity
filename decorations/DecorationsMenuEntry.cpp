// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <sigc++/adaptors/hide.h>
#include "DecorationsMenuEntry.h"
#include "DecorationStyle.h"
#include "UnitySettings.h"

namespace unity
{
namespace decoration
{

using namespace indicator;

MenuEntry::MenuEntry(Entry::Ptr const& entry, CompWindow* win)
  : horizontal_padding(5)
  , vertical_padding(3)
  , active(entry->active())
  , show_now(entry->show_now())
  , in_dropdown(false)
  , entry_(entry)
  , grab_(win, true)
  , show_menu_enabled_(true)
{
  entry_->updated.connect(sigc::mem_fun(this, &MenuEntry::EntryUpdated));
  in_dropdown.changed.connect([this] (bool in) { visible = entry_->visible() && !in; });

  auto render_texture_cb = sigc::hide(sigc::mem_fun(this, &MenuEntry::RenderTexture));
  horizontal_padding.changed.connect(render_texture_cb);
  vertical_padding.changed.connect(render_texture_cb);
  scale.changed.connect(render_texture_cb);
  focused.changed.connect(render_texture_cb);
  Style::Get()->font.changed.connect(render_texture_cb);

  EntryUpdated();
}

std::string const& MenuEntry::Id() const
{
  return entry_->id();
}

void MenuEntry::EntryUpdated()
{
  sensitive = entry_->label_sensitive() || entry_->image_sensitive();
  visible = entry_->visible() && !in_dropdown();
  active = entry_->active();
  show_now = entry_->show_now();

  RenderTexture();
}

void MenuEntry::RenderTexture()
{
  WidgetState state = focused() ? WidgetState::NORMAL : WidgetState::BACKDROP;

  if (show_now())
    state = WidgetState::PRESSED;

  if (active())
    state = WidgetState::PRELIGHT;

  natural_ = Style::Get()->MenuItemNaturalSize(entry_->label());
  cu::CairoContext text_ctx(GetNaturalWidth(), GetNaturalHeight(), scale());

  if (state == WidgetState::PRELIGHT)
    Style::Get()->DrawMenuItem(state, text_ctx, text_ctx.width() / scale(), text_ctx.height() / scale());

  nux::Rect bg_geo(-(horizontal_padding()*scale()), -(vertical_padding()*scale()), GetNaturalWidth(), GetNaturalHeight());

  if (state != WidgetState::PRELIGHT)
  {
    if (BasicContainer::Ptr const& top = GetTopParent())
    {
      auto const& top_geo = top->Geometry();
      auto const& geo = Geometry();
      bg_geo.Set(top_geo.x() - geo.x(), top_geo.y() - geo.y(), top_geo.width(), top_geo.height());
    }
  }

  cairo_save(text_ctx);
  cairo_translate(text_ctx, horizontal_padding(), vertical_padding());
  Style::Get()->DrawMenuItemEntry(entry_->label(), state, text_ctx, natural_.width, natural_.height, bg_geo * (1.0/scale));
  cairo_restore(text_ctx);
  SetTexture(text_ctx);
}

void MenuEntry::ShowMenu(unsigned button)
{
  if (active)
    return;

  active = true;
  auto const& geo = Geometry();
  entry_->ShowMenu(grab_.Window()->id(), geo.x(), geo.y2(), button);
}

int MenuEntry::GetNaturalWidth() const
{
  return (natural_.width + horizontal_padding() * 2) * scale();
}

int MenuEntry::GetNaturalHeight() const
{
  return (natural_.height + vertical_padding() * 2) * scale();
}

void MenuEntry::ButtonDownEvent(CompPoint const& p, unsigned button, Time timestamp)
{
  button_up_timer_.reset();
  grab_.ButtonDownEvent(p, button, timestamp);
  show_menu_enabled_ = (focused() || Settings::Instance().lim_unfocused_popup());
}

void MenuEntry::ButtonUpEvent(CompPoint const& p, unsigned button, Time timestamp)
{
  if (!show_menu_enabled_)
  {
    grab_.ButtonUpEvent(p, button, timestamp);
    return;
  }

  if (button == 1 && !grab_.IsGrabbed())
  {
    unsigned double_click_wait = Settings::Instance().lim_double_click_wait();
    if (grab_.IsMaximizable() && double_click_wait > 0)
    {
      button_up_timer_.reset(new glib::Timeout(double_click_wait));
      button_up_timer_->Run([this, button] {
        ShowMenu(button);
        return false;
      });
    }
    else
    {
      ShowMenu(button);
    }
  }

  if (button == 2 || button == 3)
  {
    if (Style::Get()->WindowManagerAction(WMEvent(button)) == WMAction::NONE)
      ShowMenu(button);
  }

  grab_.ButtonUpEvent(p, button, timestamp);
}

void MenuEntry::MotionEvent(CompPoint const& p, Time timestamp)
{
  bool ignore_movement = false;

  if (!grab_.IsGrabbed())
  {
    if (Geometry().contains(p))
    {
      int move_threshold = Settings::Instance().lim_movement_thresold();
      auto const& clicked = grab_.ClickedPoint();

      if (std::abs(p.x() - clicked.x()) < move_threshold &&
          std::abs(p.y() - clicked.y()) < move_threshold)
      {
        ignore_movement = true;
      }
    }
  }

  if (!ignore_movement)
    grab_.MotionEvent(p, timestamp);
}

indicator::Entry::Ptr const& MenuEntry::GetEntry() const
{
  return entry_;
}

void MenuEntry::AddProperties(debug::IntrospectionData& data)
{
  TexturedItem::AddProperties(data);
  data.add("entry_id", Id())
  .add("label", entry_->label())
  .add("label_visible", entry_->label_visible())
  .add("label_sensitive", entry_->label_sensitive())
  .add("active", entry_->active())
  .add("in_dropdown", in_dropdown());
}

debug::Introspectable::IntrospectableList MenuEntry::GetIntrospectableChildren()
{
  return IntrospectableList({&grab_});
}

} // decoration namespace
} // unity namespace
