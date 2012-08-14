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
 *              Jay Taoko <jay.taoko@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef QUICKLISTMENUITEMLABEL_H
#define QUICKLISTMENUITEMLABEL_H

#include "QuicklistMenuItem.h"

namespace unity
{

class QuicklistMenuItemLabel : public QuicklistMenuItem
{
public:
  QuicklistMenuItemLabel(glib::Object<DbusmenuMenuitem> const& item, NUX_FILE_LINE_PROTO);

protected:
  std::string GetName() const;

  virtual std::string GetDefaultText() const;
  virtual void UpdateTexture();
};

} // NAMESPACE

#endif // QUICKLISTMENUITEMLABEL_H
