// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2012 Canonical Ltd
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
 *              Michal Hruby <michal.hruby@canonical.com>
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
#include "GLibDBusProxy.h"
#include "Variant.h"
#include "Result.h"
#include "ScopeProxyInterface.h"

namespace unity
{
namespace dash
{

class Scope;

enum LayoutHint
{
  NONE,
  LEFT,
  RIGHT,
  TOP,
  BOTTOM
};

class Preview : public sigc::trackable
{
public:
  struct Action
  {
    std::string id;
    std::string display_name;
    std::string icon_hint;
    std::string extra_text;
    std::string activation_uri;
    LayoutHint layout_hint;
    // TODO: there's also a HashTable here (although unused atm)

    Action() {};
    Action(const gchar* id_, const gchar* display_name_,
           const gchar* icon_hint_, LayoutHint layout_hint_,
           GHashTable* hints)
      : id(id_ != NULL ? id_ : "")
      , display_name(display_name_ != NULL ? display_name_ : "")
      , icon_hint(icon_hint_ != NULL ? icon_hint_ : "")
      , layout_hint(layout_hint_)
    {
      GHashTableIter iter;
      gpointer key, value;
      g_hash_table_iter_init(&iter, hints);
      while (g_hash_table_iter_next(&iter, &key, &value))
      {
        if (g_strcmp0((gchar*)key, "extra-text") == 0)
        {
          glib::Variant val(static_cast<GVariant*>(value));
          extra_text = val.GetString();
        }
        else if (g_strcmp0((gchar*)key, "activation-uri") == 0)
        {
          glib::Variant val(static_cast<GVariant*>(value));
          activation_uri = val.GetString();                    
        }
      }
    };
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

  static Preview::Ptr PreviewForVariant(glib::Variant const& properties);
  static Preview::Ptr PreviewForProtocolObject(glib::Object<GObject> const& proto_obj);

  nux::ROProperty<std::string> renderer_name;
  nux::ROProperty<std::string> title;
  nux::ROProperty<std::string> subtitle;
  nux::ROProperty<std::string> description;
  nux::ROProperty<unity::glib::Object<GIcon>> image;
  nux::ROProperty<std::string> image_source_uri;

  // can't use Scope::Ptr to avoid circular dependency
  nux::RWProperty<Scope*> parent_scope;
  LocalResult preview_result;

  ActionPtrList const& GetActions() const;
  ActionPtr GetActionById(std::string const& id) const;
  InfoHintPtrList const& GetInfoHints() const;

  void PerformAction(std::string const& id,
                     glib::HintsMap const& hints = glib::HintsMap(),
                     ActivateCallback const& callback = nullptr,
                     GCancellable* cancellable = nullptr) const;

protected:
  // this should be UnityProtocolPreview, but we want to keep the usage
  // of libunity-protocol-private private to unity-core
  Preview(glib::Object<GObject> const& proto_obj);

private:
  class Impl;
  std::unique_ptr<Impl> pimpl;
};

}
}

#endif
