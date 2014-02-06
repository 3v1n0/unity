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

#ifndef UNITY_DECORATIONS_MENU_ENTRY
#define UNITY_DECORATIONS_MENU_ENTRY

#include <UnityCore/IndicatorEntry.h>
#include "DecorationsGrabEdge.h"

namespace unity
{
namespace decoration
{

class MenuEntry : public TexturedItem
{
public:
  typedef std::shared_ptr<MenuEntry> Ptr;
  typedef unity::uweak_ptr<MenuEntry> WeakPtr;

  MenuEntry(indicator::Entry::Ptr const&, CompWindow*);

  nux::Property<bool> active;
  std::string const& Id() const;

  void ShowMenu(unsigned button);

protected:
  std::string GetName() const override { return "MenuEntry"; }
  void AddProperties(debug::IntrospectionData&) override;
  IntrospectableList GetIntrospectableChildren() override;

  int GetNaturalWidth() const override;
  int GetNaturalHeight() const override;
  void ButtonDownEvent(CompPoint const&, unsigned button) override;
  void ButtonUpEvent(CompPoint const&, unsigned button) override;
  void MotionEvent(CompPoint const&) override;

private:
  void RebuildTexture();

  glib::Source::UniquePtr button_up_timer_;
  indicator::Entry::Ptr entry_;
  GrabEdge grab_;
  nux::Size real_size_;
};

} // decoration namespace
} // unity namespace

#endif // UNITY_DECORATIONS_MENU_ENTRY
