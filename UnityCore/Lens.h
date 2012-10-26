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

#ifndef UNITY_LENS_H
#define UNITY_LENS_H

#include <boost/noncopyable.hpp>
#include <memory>
#include <NuxCore/Property.h>
#include <sigc++/trackable.h>

#include "Variant.h"
#include "GLibDBusProxy.h"
#include "Categories.h"
#include "Filters.h"
#include "Preview.h"
#include "Results.h"

namespace unity
{
namespace dash
{

enum HandledType
{
  NOT_HANDLED=0,
  SHOW_DASH,
  HIDE_DASH,
  GOTO_DASH_URI
};

enum ViewType
{
  HIDDEN=0,
  HOME_VIEW,
  LENS_VIEW
};

class Lens : public sigc::trackable, boost::noncopyable
{
public:
  typedef std::shared_ptr<Lens> Ptr;
  typedef std::map<std::string, unity::glib::Variant> Hints;
  typedef std::function<void(Hints const&, glib::Error const&)> SearchFinishedCallback;

  Lens(std::string const& id,
       std::string const& dbus_name,
       std::string const& dbus_path,
       std::string const& name,
       std::string const& icon,
       std::string const& description = "",
       std::string const& search_hint = "",
       bool visible = true,
       std::string const& shortcut = "");

  Lens(std::string const& id,
         std::string const& dbus_name,
         std::string const& dbus_path,
         std::string const& name,
         std::string const& icon,
         std::string const& description,
         std::string const& search_hint,
         bool visible,
         std::string const& shortcut,
         ModelType model_type);

  virtual ~Lens();

  virtual void GlobalSearch(std::string const& search_string, SearchFinishedCallback cb = nullptr);
  virtual void Search(std::string const& search_string, SearchFinishedCallback cb = nullptr);
  virtual void Activate(std::string const& uri);
  virtual void Preview(std::string const& uri);
  virtual void ActivatePreviewAction(std::string const& action_id,
                                     std::string const& uri,
                                     Hints const& hints);
  virtual void SignalPreview(std::string const& uri,
      glib::Variant const& preview_update,
      glib::DBusProxy::ReplyCallback reply_cb = nullptr);

  /**
   * Note that this model is only valid for as long as the results model
   * doesn't change.
   * (you should call this again after models_changed is emitted)
   */
  virtual glib::Object<DeeModel> GetFilterModelForCategory(unsigned category);
  virtual std::vector<unsigned> GetCategoriesOrder();

  nux::RWProperty<std::string> id;
  nux::RWProperty<std::string> dbus_name;
  nux::RWProperty<std::string> dbus_path;
  nux::RWProperty<std::string> name;
  nux::RWProperty<std::string> icon_hint;
  nux::RWProperty<std::string> description;
  nux::RWProperty<std::string> search_hint;
  nux::RWProperty<bool> visible;
  nux::RWProperty<bool> search_in_global;
  nux::RWProperty<std::string> shortcut;
  nux::RWProperty<Results::Ptr> results;
  nux::RWProperty<Results::Ptr> global_results;
  nux::RWProperty<Categories::Ptr> categories;
  nux::RWProperty<Filters::Ptr> filters;
  nux::RWProperty<bool> connected;
  nux::RWProperty<bool> provides_personal_content;
  nux::ROProperty<std::string> last_search_string;
  nux::ROProperty<std::string> last_global_search_string;

  nux::Property<ViewType> view_type;

  sigc::signal<void> categories_reordered;
  /* Emitted when any of the models' swarm name changes, but collates the name
   * changes into a single signal emission (when all are changed) */
  sigc::signal<void> models_changed;
  //sigc::signal<void, Hints const&> search_finished;
  //sigc::signal<void, Hints const&> global_search_finished;
  sigc::signal<void, std::string const&, HandledType, Hints const&> activated;
  sigc::signal<void, std::string const&, Preview::Ptr const&> preview_ready;

private:
  class Impl;
  Impl* pimpl;
};

}
}

#endif
