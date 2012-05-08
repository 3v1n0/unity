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
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 */


#ifndef UNITYSHELL_ABSTRACTPLACESGROUP_H
#define UNITYSHELL_ABSTRACTPLACESGROUP_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <NuxCore/Property.h>

namespace unity
{
namespace dash
{

class AbstractPlacesGroup : public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(AbstractPlacesGroup, nux::View);
public:
  AbstractPlacesGroup();

  // Properties
  nux::Property<bool> draw_separator;

protected:
  void Draw(nux::GraphicsEngine&, bool);
  void DrawContent(nux::GraphicsEngine&, bool);

};

}
}

#endif

