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
  struct Action
  {
    std::string id;
    std::string display_name;
    std::string icon_hint;
    unsigned int layout_hint;
    // TODO: there's also a HashTable here (although unused atm)

    Action() {};
    Action(const gchar* id_, const gchar* display_name_,
           const gchar* icon_hint_, unsigned int layout_hint_)
      : id(id_ != NULL ? id_ : "")
      , display_name(display_name_ != NULL ? display_name_ : "")
      , icon_hint(icon_hint_ != NULL ? icon_hint_ : "")
      , layout_hint(layout_hint_) {};
  };

  struct InfoHint
  {
    std::string id;
    std::string display_name;
    std::string icon_hint;
    unity::glib::Variant value;

    InfoHint() {};
    InfoHint(const gchar* id_, const gchar* display_name_, 
             const gchar* icon_hint_, GVariant* value_)
      : id(id_ != NULL ? id_ : "")
      , display_name(display_name_ != NULL ? display_name_ : "")
      , icon_hint(icon_hint_ != NULL ? icon_hint_ : "")
      , value(value_) {};
  };

  typedef std::shared_ptr<Preview> Ptr;
  typedef std::shared_ptr<Action> ActionPtr;
  typedef std::shared_ptr<InfoHint> InfoHintPtr;
  typedef std::vector<ActionPtr> ActionPtrList;
  typedef std::vector<InfoHintPtr> InfoHintPtrList;

  virtual ~Preview();

  static Preview::Ptr PreviewForVariant(unity::glib::Variant& properties);

  nux::RWProperty<std::string> renderer_name;
  nux::RWProperty<std::string> title;
  nux::RWProperty<std::string> subtitle;
  nux::RWProperty<std::string> description;
  nux::RWProperty<unity::glib::Object<GIcon>> image;

  ActionPtrList GetActions() const;
  InfoHintPtrList GetInfoHints() const;

protected:
  // this should be UnityProtocolPreview, but we want to keep the usage
  // of libunity-protocol-private private to unity-core
  Preview(unity::glib::Object<GObject> const& proto_obj);
  static unity::glib::Object<GIcon> IconForString(std::string const& icon_hint);

private:
  class Impl;
  std::unique_ptr<Impl> pimpl;
};

}
}

#endif
