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

#ifndef UNITY_FILTER_H
#define UNITY_FILTER_H

#include <dee.h>
#include <map>
#include <memory>
#include <NuxCore/Property.h>
#include <sigc++/sigc++.h>

#include "GLibSignal.h"
#include "Variant.h"

namespace unity
{
namespace dash
{

enum FilterColumn
{
  ID = 0,
  NAME,
  ICON_HINT,
  RENDERER_NAME,
  RENDERER_STATE,
  VISIBLE,
  COLLAPSED,
  FILTERING
};

class FilterOption : public sigc::trackable
{
public:
  typedef std::shared_ptr<FilterOption> Ptr;

  FilterOption(std::string id_, std::string name_, std::string icon_hint_, bool active_)
    : id(id_)
    , name(name_)
    , icon_hint(icon_hint_)
    , active(active_)
  {}
  nux::Property<std::string> id;
  nux::Property<std::string> name;
  nux::Property<std::string> icon_hint;
  nux::Property<bool> active;
};

class Filter : public sigc::trackable
{
public:
  typedef std::shared_ptr<Filter> Ptr;
  typedef std::map<std::string, unity::glib::Variant> Hints;

  Filter(DeeModel* model, DeeModelIter* iter);
  virtual ~Filter();

  static Filter::Ptr FilterFromIter(DeeModel* model, DeeModelIter* iter);

  virtual void Clear() = 0;
  bool IsValid() const;

  glib::Variant VariantValue() const;

  nux::ROProperty<std::string> id;
  nux::ROProperty<std::string> name;
  nux::ROProperty<std::string> icon_hint;
  nux::ROProperty<std::string> renderer_name;
  nux::ROProperty<bool> visible;
  nux::ROProperty<bool> collapsed;
  nux::ROProperty<bool> filtering;

  sigc::signal<void> removed;

protected:
  virtual void Update(Hints& hints) = 0;
  void Refresh();
  void IgnoreChanges(bool ignore);

private:
  void SetupGetters();
  static void OnModelDestroyed(Filter* self, DeeModel* old_location);
  void OnRowChanged(DeeModel* model, DeeModelIter* iter);
  void OnRowRemoved(DeeModel* model, DeeModelIter* iter);
  void HintsToMap(Hints& hints);

  std::string get_id() const;
  std::string get_name() const;
  std::string get_icon_hint() const;
  std::string get_renderer_name() const;
  bool get_visible() const;
  bool get_collapsed() const;
  bool get_filtering() const;

protected:
  DeeModel* model_;
  DeeModelIter* iter_;
  glib::SignalManager signal_manager_;
  bool ignore_changes_;
};

}
}

#endif
