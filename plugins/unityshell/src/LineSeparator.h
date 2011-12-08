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
 * Authored by: Jay Taoko <jaytaoko@inalogic.com>
 *
 */

#ifndef UNITYSHELL_HSEPARATOR_H
#define UNITYSHELL_HSEPARATOR_H

#include "AbstractSeparator.h"

namespace unity
{

class HSeparator: public AbstractSeparator
{
public:
  HSeparator();
  HSeparator(nux::Color const& color, float Alpha0, float Alpha1, int Border);

  ~HSeparator();

protected:
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {};

};

} // namespace unity

#endif // UNITYSHELL_HSEPARATOR_H
