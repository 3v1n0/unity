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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "PlaceFactory.h"
#include "PlaceFactoryFile.h"

static PlaceFactory* default_factory = NULL;

PlaceFactory*
PlaceFactory::GetDefault()
{
  if (!default_factory)
    default_factory = new PlaceFactoryFile();

  return default_factory;
}

void
PlaceFactory::SetDefault(PlaceFactory* factory)
{
  if (default_factory)
    delete default_factory;

  default_factory = factory;
}
