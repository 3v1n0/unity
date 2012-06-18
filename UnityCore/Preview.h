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
#include <string>
#include <vector>

#include <glib.h>
#include <gio/gio.h>
#include <sigc++/trackable.h>
#include <NuxCore/Property.h>

#include "GLibWrapper.h"
#include "Variant.h"

namespace unity
{
namespace dash
{

class Preview : public sigc::trackable
{
public:
  typedef std::shared_ptr<Preview> Ptr;

  virtual ~Preview();

  static Preview::Ptr PreviewForVariant(unity::glib::Variant& properties);

  nux::RWProperty<std::string> title;
  nux::RWProperty<std::string> subtitle;
  nux::RWProperty<std::string> description;
  nux::RWProperty<unity::glib::Object<GIcon>> thumbnail;

  // TODO: actions, info hints
protected:
  // this should be UnityProtocolPreview, but we want to keep the usage
  // of libunity-protocol-private private to unity-core
  Preview(unity::glib::Object<GObject> const& proto_obj);

  virtual void SetupGetters();
  static unity::glib::Object<GIcon> IconForString(std::string const& icon_hint);

private:
  std::string get_title() const { return title_; };
  std::string get_subtitle() const { return subtitle_; };
  std::string get_description() const { return description_; };
  unity::glib::Object<GIcon> get_thumbnail() const { return thumbnail_; };

  std::string title_;
  std::string subtitle_;
  std::string description_;
  unity::glib::Object<GIcon> thumbnail_;
};

}
}

#endif
