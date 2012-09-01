// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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

#ifndef LAUNCHERMODEL_H
#define LAUNCHERMODEL_H

#include <UnityCore/GLibSource.h>
#include "AbstractLauncherIcon.h"
#include "unity-shared/Introspectable.h"

namespace unity
{
namespace launcher
{

class LauncherModel : public unity::debug::Introspectable, public sigc::trackable
{
public:
  typedef std::shared_ptr<LauncherModel> Ptr;
  typedef std::vector<AbstractLauncherIcon::Ptr> Base;
  typedef Base::iterator iterator;
  typedef Base::const_iterator const_iterator;
  typedef Base::reverse_iterator reverse_iterator;
  typedef Base::reverse_iterator const_reverse_iterator;

  LauncherModel();

  void AddIcon(AbstractLauncherIcon::Ptr icon);
  void RemoveIcon(AbstractLauncherIcon::Ptr icon);
  void Save();
  void Sort();
  int  Size() const;

  void OnIconRemove(AbstractLauncherIcon::Ptr icon);

  bool IconHasSister(AbstractLauncherIcon::Ptr icon) const;

  void ReorderAfter(AbstractLauncherIcon::Ptr icon, AbstractLauncherIcon::Ptr other);
  void ReorderBefore(AbstractLauncherIcon::Ptr icon, AbstractLauncherIcon::Ptr other, bool save);

  void ReorderSmart(AbstractLauncherIcon::Ptr icon, AbstractLauncherIcon::Ptr other, bool save);

  AbstractLauncherIcon::Ptr Selection() const;
  int SelectionIndex() const;
  void SetSelection(int selection);
  void SelectNext();
  void SelectPrevious();

  AbstractLauncherIcon::Ptr GetClosestIcon(AbstractLauncherIcon::Ptr icon, bool& is_before) const;

  iterator begin();
  iterator end();
  iterator at(int index);
  reverse_iterator rbegin();
  reverse_iterator rend();

  iterator main_begin();
  iterator main_end();
  reverse_iterator main_rbegin();
  reverse_iterator main_rend();

  iterator shelf_begin();
  iterator shelf_end();
  reverse_iterator shelf_rbegin();
  reverse_iterator shelf_rend();

  sigc::signal<void, AbstractLauncherIcon::Ptr> icon_added;
  sigc::signal<void, AbstractLauncherIcon::Ptr> icon_removed;
  sigc::signal<void> order_changed;
  sigc::signal<void> saved;
  sigc::signal<void, AbstractLauncherIcon::Ptr> selection_changed;

  IntrospectableList GetIntrospectableChildren();
protected:
  // Introspectable methods
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  Base             _inner;
  Base             _inner_shelf;
  Base             _inner_main;
  int              selection_;
  std::list<unity::debug::Introspectable*> introspection_results_;
  glib::SourceManager timeouts_;

  bool Populate();

  bool IconShouldShelf(AbstractLauncherIcon::Ptr icon) const;

  static bool CompareIcons(AbstractLauncherIcon::Ptr first, AbstractLauncherIcon::Ptr second);

  /* Template Methods */
public:
  template<class T>
  std::list<AbstractLauncherIcon::Ptr> GetSublist()
  {
    std::list<AbstractLauncherIcon::Ptr> result;

    iterator it;
    for (it = begin(); it != end(); it++)
    {
      T* var = dynamic_cast<T*>((*it).GetPointer());

      if (var)
        result.push_back(*it);
    }

    return result;
  }
};

}
}

#endif // LAUNCHERMODEL_H

