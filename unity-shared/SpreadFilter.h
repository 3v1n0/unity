// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2014 Canonical Ltd
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
* Authored by: Marco Trevisan <marco@ubuntu.com>
*/

#ifndef UNITYSHELL_SPREAD_FILTER_H
#define UNITYSHELL_SPREAD_FILTER_H

#include <memory>

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <NuxCore/Animation.h>
#include "Introspectable.h"

namespace unity
{
class SearchBar;

namespace spread
{

class Filter : public debug::Introspectable, public sigc::trackable
{
public:
  typedef std::shared_ptr<Filter> Ptr;

  Filter();
  virtual ~Filter();

  nux::RWProperty<std::string> text;

  bool Visible() const;
  nux::Geometry const& GetAbsoluteGeometry() const;

protected:
  // Introspectable
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

private:
  nux::ObjectPtr<SearchBar> search_bar_;
  nux::ObjectPtr<nux::BaseWindow> view_window_;
  nux::animation::AnimateValue<double> fade_animator_;
};

} // namespace spread
} // namespace unity

#endif
