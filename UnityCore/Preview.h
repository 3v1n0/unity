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

#ifndef UNITY_PREVIEW_H
#define UNITY_PREVIEW_H

#include <map>
#include <memory>
#include <vector>

#include <sigc++/trackable.h>
#include <NuxCore/Property.h>

#include <glib.h>

namespace unity
{
namespace dash
{

class Preview : public sigc::trackable
{
public:
  typedef std::shared_ptr<Preview> Ptr;
  typedef std::map<std::string, GVariant*> Properties;

  virtual ~Preview();

  static Preview::Ptr PreviewForProperties(std::string const& renderer_name, Properties& properties);

  nux::Property<std::string> renderer_name;

protected:
  unsigned int PropertyToUnsignedInt (Properties& properties, const char* key);
  std::string PropertyToString(Properties& properties, const char *key);
  std::vector<std::string> PropertyToStringVector(Properties& properties, const char *key);
};

class NoPreview : public Preview
{
public:
  NoPreview();
};

}
}

#endif
