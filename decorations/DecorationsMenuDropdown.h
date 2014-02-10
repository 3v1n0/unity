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

#ifndef UNITY_DECORATIONS_MENU_DROPDOWN
#define UNITY_DECORATIONS_MENU_DROPDOWN

#include <UnityCore/Indicators.h>
#include "DecorationsMenuEntry.h"

namespace unity
{
namespace decoration
{

class MenuDropdown : public MenuEntry
{
public:
  MenuDropdown(indicator::Indicators::Ptr const&, CompWindow*);

  void ShowMenu(unsigned button) override;

  void Push(MenuEntry::Ptr const&);
  MenuEntry::Ptr Pop();
  MenuEntry::Ptr Top() const;

  size_t Size() const;
  bool Empty() const;

protected:
  std::string GetName() const override { return "MenuDropdown"; }
  IntrospectableList GetIntrospectableChildren() override;

private:
  void RenderTexture() override;

  indicator::Indicators::Ptr indicators_;
  std::deque<MenuEntry::Ptr> children_;
};

} // decoration namespace
} // unity namespace

#endif // UNITY_DECORATIONS_MENU_DROPDOWN
