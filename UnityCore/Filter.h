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

#include <boost/shared_ptr.hpp>
#include <dee.h>
#include <map>
#include <NuxCore/Property.h>
#include <sigc++/trackable.h>

#include "GLibSignal.h"

namespace unity {
namespace dash {

class Filter : public sigc::trackable
{
public:
  typedef boost::shared_ptr<Filter> Ptr;
  typedef std::map<std::string, GVariant*> Hints;

  Filter();
  virtual ~Filter();

  static Filter::Ptr FilterFromIter(DeeModel* model, DeeModelIter* iter);

  virtual void Clear();

  nux::Property<std::string> id;
  nux::Property<std::string> name;
  nux::Property<std::string> icon_hint;
  nux::Property<std::string> renderer_name;
  nux::Property<bool> visible;
  nux::Property<bool> collapsed;
  nux::Property<bool> filtering;

  sigc::signal<void> removed;

protected:
  void Connect();
  virtual void Update(Hints& hints);

private:
  static void OnModelDestroyed(Filter* self, DeeModel* old_location);
  void OnRowChanged(DeeModel* model, DeeModelIter* iter);
  void OnRowRemoved(DeeModel* model, DeeModelIter* iter);
  void UpdateProperties();
  void HintsToMap(Hints& hints);
  
protected:
  DeeModel* model_;
  DeeModelIter* iter_;
  glib::SignalManager signal_manager_;
};

}
}

#endif
