// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef QUICKLISTMENUITEMSEPARATOR_H
#define QUICKLISTMENUITEMSEPARATOR_H

#include "QuicklistMenuItem.h"

namespace unity
{

class QuicklistMenuItemSeparator : public QuicklistMenuItem
{
public:
  QuicklistMenuItemSeparator(glib::Object<DbusmenuMenuitem> const& item, NUX_FILE_LINE_PROTO);

  virtual bool GetSelectable();

protected:
  void Draw(nux::GraphicsEngine& gfxContext, bool forceDraw);
  std::string GetName() const;

  virtual void UpdateTexture();

private:
  nux::Color _color;
  nux::Color _premultiplied_color;
};

}
#endif // QUICKLISTMENUITEMSEPARATOR_H
