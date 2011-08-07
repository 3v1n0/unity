// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "ApplicationPreview.h"

namespace unity
{
namespace dash
{

ApplicationPreview::ApplicationPreview(Preview::Properties& properties)
  : name(PropertyToString(properties, "name"))
  , version(PropertyToString(properties, "version"))
  , size(PropertyToString(properties, "size"))
  , license(PropertyToString(properties, "license"))
  , last_updated(PropertyToString(properties, "last-updated"))
  , rating(PropertyToFloat(properties, "rating"))
  , n_ratings(PropertyToUnsignedInt(properties, "n-ratings"))
  , description(PropertyToString(properties, "string"))
  , icon_hint(PropertyToString(properties, "icon-hint"))
  , screenshot_icon_hint(PropertyToString(properties, "screenshot-icon-hint"))
  , primary_action_name(PropertyToString(properties, "primary-action-name"))
  , primary_action_icon_hint(PropertyToString(properties, "primary-action-icon-hint"))
  , primary_action_uri(PropertyToString(properties, "primary-action-uri"))
{
 renderer_name = "preview-application";
}


}
}
