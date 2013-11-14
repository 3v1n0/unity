/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Gordon Allott <gord.alott@canonical.com>
 */

#ifndef BGHASH_H
#define BGHASH_H

#include <NuxCore/Animation.h>
#include <Nux/Nux.h>

namespace unity {
namespace colors {
  const nux::Color Aubergine(0x3E, 0x20, 0x60);
};
};

namespace unity
{
  class BGHash
  {
  public:
    BGHash();

    nux::Color CurrentColor() const;
    uint64_t ColorAtomId() const;
    void RefreshColor();
    void OverrideColor(nux::Color const& color);

  private:
    void OnTransitionUpdated(nux::Color const& new_color);
    void TransitionToNewColor(nux::Color const& new_color);
    nux::Color MatchColor(nux::Color const& base_color) const;

  private:
    nux::animation::AnimateValue<nux::Color> transition_animator_;
    nux::Color override_color_;
  };
};

#endif // BGHASH_H
