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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#ifndef SWITCHERMODEL_H
#define SWITCHERMODEL_H

#include <sys/time.h>

#include "AbstractLauncherIcon.h"
#include "LauncherModel.h"

#include "unity-shared/Introspectable.h"

#include <boost/shared_ptr.hpp>
#include <sigc++/sigc++.h>

namespace unity
{
namespace switcher
{

class SwitcherModel : public debug::Introspectable, public sigc::trackable
{

public:
  typedef boost::shared_ptr<SwitcherModel> Ptr;

  typedef std::vector<launcher::AbstractLauncherIcon::Ptr> Base;
  typedef Base::iterator iterator;
  typedef Base::reverse_iterator reverse_iterator;

  nux::Property<bool> detail_selection;
  nux::Property<unsigned int> detail_selection_index;
  nux::Property<bool> only_detail_on_viewport;

  // Icons are owned externally and assumed valid for life of switcher.
  // When AbstractLauncherIcon is complete, it will be passed as a shared pointer and this
  // will no longer be a worry.
  SwitcherModel(std::vector<launcher::AbstractLauncherIcon::Ptr> icons);
  virtual ~SwitcherModel();

  iterator begin();
  iterator end();

  reverse_iterator rbegin();
  reverse_iterator rend();

  launcher::AbstractLauncherIcon::Ptr at(unsigned int index);

  int Size();

  launcher::AbstractLauncherIcon::Ptr Selection();
  int SelectionIndex();

  launcher::AbstractLauncherIcon::Ptr LastSelection();
  int LastSelectionIndex();

  std::vector<Window> DetailXids ();
  Window DetailSelectionWindow ();

  void Next();
  void Prev();

  void NextDetail();
  void PrevDetail();

  void Select(launcher::AbstractLauncherIcon::Ptr const& selection);
  void Select(unsigned int index);

  sigc::signal<void, launcher::AbstractLauncherIcon::Ptr const&> selection_changed;

protected:
  // Introspectable methods
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:

  static bool CompareWindowsByActive (guint32 first, guint32 second);

  Base             _inner;
  unsigned int     _index;
  unsigned int     _last_index;

  launcher::AbstractLauncherIcon::Ptr _last_active_icon;
};

}
}

#endif // SWITCHERMODEL_H

