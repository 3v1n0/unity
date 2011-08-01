/*
 * Copyright 2010 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */


#include "config.h"

#include "PlaceFactoryFile.h"

static void TestAllocation(void);

void
TestPlaceFactoryFileCreateSuite()
{
#define _DOMAIN "/Unit/PlaceFactoryFile"

  if (0)
    g_test_add_func(_DOMAIN"/Allocation", TestAllocation);
}

static void
TestAllocation()
{
  PlaceFactoryFile* factory;

  factory = new PlaceFactoryFile();
  g_assert(factory);

//  while (!factory->read_directory)
  while (1)
    g_main_context_iteration(NULL, TRUE);

  delete factory;
}
