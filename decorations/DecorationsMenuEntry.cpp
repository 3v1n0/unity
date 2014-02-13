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

namespace unity
{
namespace decoration
{
namespace
{
const unsigned MAX_DOUBLE_CLICK_WAIT = 100;
const int MOVEMENT_THRESHOLD = 4;
}

using namespace indicator;

MenuEntry::MenuEntry(Entry::Ptr const& entry, CompWindow* win)
  : horizontal_padding(5)
  , vertical_padding(3)
  , active(entry->active())
  , show_now(entry->show_now())
  , in_dropdown(false)
  , entry_(entry)
  , grab_(win, true)
{
  entry_->updated.connect(sigc::mem_fun(this, &MenuEntry::EntryUpdated));
  horizontal_padding.changed.connect(sigc::hide(sigc::mem_fun(this, &MenuEntry::RenderTexture)));
  vertical_padding.changed.connect(sigc::hide(sigc::mem_fun(this, &MenuEntry::RenderTexture)));
  in_dropdown.changed.connect([this] (bool in) { visible = entry_->visible() && !in; });
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
  WidgetState state = WidgetState::NORMAL;

  if (show_now())
    state = WidgetState::PRESSED;

  if (active())
    state = WidgetState::PRELIGHT;

  natural_ = Style::Get()->MenuItemNaturalSize(entry_->label());
  cu::CairoContext text_ctx(GetNaturalWidth(), GetNaturalHeight());

  if (state == WidgetState::PRELIGHT)
    Style::Get()->DrawMenuItem(state, text_ctx, text_ctx.width(), text_ctx.height());

  cairo_save(text_ctx);
  cairo_translate(text_ctx, horizontal_padding(), vertical_padding());
  Style::Get()->DrawMenuItemEntry(entry_->label(), state, text_ctx, natural_.width, natural_.height);
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
  return natural_.width + horizontal_padding() * 2;
}

int MenuEntry::GetNaturalHeight() const
{
  return natural_.height + vertical_padding() * 2;
}

void MenuEntry::ButtonDownEvent(CompPoint const& p, unsigned button, Time timestamp)
{
  button_up_timer_.reset();
  grab_.ButtonDownEvent(p, button, timestamp);
}

void MenuEntry::ButtonUpEvent(CompPoint const& p, unsigned button, Time timestamp)
{
  if (button == 1 && !grab_.IsGrabbed())
  {
    if (grab_.IsMaximizable())
    {
      button_up_timer_.reset(new glib::Timeout(MAX_DOUBLE_CLICK_WAIT));
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
    ShowMenu(button);
  }

  grab_.ButtonUpEvent(p, button, timestamp);
}

void MenuEntry::MotionEvent(CompPoint const& p, Time timestamp)
{
  bool ignore_movement = false;

  if (!grab_.IsGrabbed())
  {
    auto const& clicked = grab_.ClickedPoint();

    if (std::abs(p.x() - clicked.x()) < MOVEMENT_THRESHOLD &&
        std::abs(p.y() - clicked.y()) < MOVEMENT_THRESHOLD)
    {
      ignore_movement = true;
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
