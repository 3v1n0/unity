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

#ifndef UNITY_DECORATIONS_SLIDING_LAYOUT
#define UNITY_DECORATIONS_SLIDING_LAYOUT

#include <NuxCore/Animation.h>
#include "DecorationsWidgets.h"

namespace unity
{
namespace decoration
{

class SlidingLayout : public BasicContainer
{
public:
  typedef std::shared_ptr<SlidingLayout> Ptr;

  SlidingLayout();

  nux::Property<unsigned> fadein;
  nux::Property<unsigned> fadeout;
  nux::Property<bool> override_main_item;

  void SetMainItem(Item::Ptr const& main);
  void SetInputItem(Item::Ptr const& input);

protected:
  void Draw(GLWindow*, GLMatrix const&, GLWindowPaintAttrib const&, CompRegion const&, unsigned mask) override;
  std::string GetName() const override { return "SlidingLayout"; }

private:
  void DoRelayout() override;
  void StartAnimation();

  nux::animation::AnimateValue<double> fade_animator_;
};

} // decoration namespace
} // unity namespace

#endif // UNITY_DECORATIONS_SLIDING_LAYOUT
