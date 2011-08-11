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

#include "GenericPreview.h"

namespace unity
{
namespace dash
{

GenericPreview::GenericPreview(Preview::Properties& properties)
  : name(PropertyToString(properties, "name"))
  , date_modified(PropertyToUnsignedInt(properties, "date-modified"))
  , size(PropertyToUnsignedInt(properties, "size"))
  , type(PropertyToString(properties, "type"))
  , description(PropertyToString(properties, "string"))
  , icon_hint(PropertyToString(properties, "icon-hint"))
  , primary_action_name(PropertyToString(properties, "primary-action-name"))
  , primary_action_icon_hint(PropertyToString(properties, "primary-action-icon-hint"))
  , primary_action_uri(PropertyToString(properties, "primary-action-uri"))
  , secondary_action_name(PropertyToString(properties, "secondary-action-name"))
  , secondary_action_icon_hint(PropertyToString(properties, "secondary-action-icon-hint"))
  , secondary_action_uri(PropertyToString(properties, "secondary-action-uri"))
  , tertiary_action_name(PropertyToString(properties, "tertiary-action-name"))
  , tertiary_action_icon_hint(PropertyToString(properties, "tertiary-action-icon-hint"))
  , tertiary_action_uri(PropertyToString(properties, "tertiary-action-uri"))
{
 renderer_name = "preview-generic";
}


}
}
