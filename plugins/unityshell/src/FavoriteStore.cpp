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
* Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
*/

#include <NuxCore/Logger.h>
#include "FavoriteStoreGSettings.h"

namespace unity
{
namespace
{
  nux::logging::Logger logger("unity.favouritestore");
  FavoriteStore* favoritestore_instance = nullptr;
}

FavoriteStore::FavoriteStore()
{
  if (favoritestore_instance)
  {
    LOG_ERROR(logger) << "More than one FavoriteStore created!";
  }
  else
  {
    favoritestore_instance = this;
  }
}

FavoriteStore::~FavoriteStore()
{
  if (favoritestore_instance == this)
    favoritestore_instance = nullptr;
}


FavoriteStore& FavoriteStore::Instance()
{
  if (! favoritestore_instance)
  {
    LOG_ERROR(logger) << "No FavoriteStore instance created yet!";
  }
  return *favoritestore_instance;
}

}
