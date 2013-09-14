// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2010-2012 Canonical Ltd
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
*              Marco Trevisan <marco.trevisan@canonical.com>
*/

#include <NuxCore/Logger.h>
#include <glib.h>

#include "FavoriteStore.h"
#include "FavoriteStorePrivate.h"

namespace unity
{
DECLARE_LOGGER(logger, "unity.favorite.store");
namespace
{
FavoriteStore* favoritestore_instance = nullptr;
const std::string PREFIX_SEPARATOR = "://";
}

const std::string FavoriteStore::URI_PREFIX_APP = "application://";
const std::string FavoriteStore::URI_PREFIX_FILE = "file://";
const std::string FavoriteStore::URI_PREFIX_DEVICE = "device://";
const std::string FavoriteStore::URI_PREFIX_UNITY = "unity://";

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

bool FavoriteStore::IsValidFavoriteUri(std::string const& uri)
{
  if (uri.empty())
    return false;

  if (uri.find(URI_PREFIX_APP) == 0 || uri.find(URI_PREFIX_FILE) == 0)
  {
    return internal::impl::IsDesktopFilePath(uri);
  }
  else if (uri.find(URI_PREFIX_DEVICE) == 0)
  {
    return uri.length() > URI_PREFIX_DEVICE.length();
  }
  else if (uri.find(URI_PREFIX_UNITY) == 0)
  {
    return uri.length() > URI_PREFIX_UNITY.length();
  }

  return false;
}

std::string FavoriteStore::ParseFavoriteFromUri(std::string const& uri) const
{
  if (uri.empty())
    return "";

  std::string fav = uri;
  auto prefix_pos = fav.find(PREFIX_SEPARATOR);

  if (prefix_pos == std::string::npos)
  {
    // We assume that favorites with no prefix, but with a .desktop suffix are applications
    if (internal::impl::IsDesktopFilePath(uri))
    {
      fav = URI_PREFIX_APP + fav;
      prefix_pos = URI_PREFIX_APP.length();
    }
  }
  else
  {
    prefix_pos += PREFIX_SEPARATOR.length();
  }

  // Matches application://desktop-id.desktop or application:///path/to/file.desktop
  if (fav.find(URI_PREFIX_APP) == 0 || fav.find(URI_PREFIX_FILE) == 0)
  {
    std::string const& fav_value = fav.substr(prefix_pos);

    if (fav_value.empty())
    {
      LOG_WARNING(logger) << "Unable to load Favorite for uri '" << fav << "'";
      return "";
    }

    if (fav_value[0] == '/' || fav.find(URI_PREFIX_FILE) == 0)
    {
      if (g_file_test(fav_value.c_str(), G_FILE_TEST_EXISTS))
      {
        return fav;
      }
      else
      {
        LOG_WARNING(logger) << "Unable to load desktop file: " << fav_value;
      }
    }
    else
    {
      return URI_PREFIX_APP + fav_value;
    }
  }
  else if (IsValidFavoriteUri(fav))
  {
    return fav;
  }

  LOG_WARNING(logger) << "Unable to load Favorite for uri '" << fav << "'";
  return "";
}

}
