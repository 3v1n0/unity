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

#include "DecorationsMenuEntry.h"
#include "DecorationStyle.h"

namespace unity
{
namespace decoration
{
namespace
{
const unsigned HORIZONTAL_PADDING = 5;
const unsigned VERTICAL_PADDING = 3;
const unsigned MAX_DOUBLE_CLICK_WAIT = 100;
}

using namespace indicator;

MenuEntry::MenuEntry(Entry::Ptr const& entry, CompWindow* win)
  : active(false)
  , in_dropdown(false)
  , entry_(entry)
  , grab_(win, true)
{
  sensitive = entry_->label_sensitive();
  visible = entry_->visible() && !in_dropdown();
  entry_->updated.connect(sigc::mem_fun(this, &MenuEntry::RebuildTexture));
  in_dropdown.changed.connect([this] (bool in) { visible = entry_->visible() && !in; });
  RebuildTexture();
}

std::string const& MenuEntry::Id() const
{
  return entry_->id();
}

void MenuEntry::RebuildTexture()
{
  sensitive = entry_->label_sensitive();
  visible = entry_->visible() && !in_dropdown();
  active = entry_->active();
  WidgetState state = WidgetState::NORMAL;

  if (entry_->show_now())
    state = WidgetState::PRESSED;

  if (active())
    state = WidgetState::PRELIGHT;

  natural_ = Style::Get()->MenuItemNaturalSize(entry_->label());
  cu::CairoContext text_ctx(GetNaturalWidth(), GetNaturalHeight());

  if (state == WidgetState::PRELIGHT)
    Style::Get()->DrawMenuItem(state, text_ctx, text_ctx.width(), text_ctx.height());

  cairo_save(text_ctx);
  cairo_translate(text_ctx, HORIZONTAL_PADDING, VERTICAL_PADDING);
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
  entry_->ShowMenu(geo.x(), geo.y2(), button);
}

int MenuEntry::GetNaturalWidth() const
{
  return natural_.width + HORIZONTAL_PADDING * 2;
}

int MenuEntry::GetNaturalHeight() const
{
  return natural_.height + VERTICAL_PADDING * 2;
}

void MenuEntry::ButtonDownEvent(CompPoint const& p, unsigned button)
{
  button_up_timer_.reset();
  grab_.ButtonDownEvent(p, button);
}

void MenuEntry::ButtonUpEvent(CompPoint const& p, unsigned button)
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

  grab_.ButtonUpEvent(p, button);
}

void MenuEntry::MotionEvent(CompPoint const& p)
{
  grab_.MotionEvent(p);
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
