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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#ifndef SWITCHERMODEL_H
#define SWITCHERMODEL_H

#include <sys/time.h>

#include "AbstractLauncherIcon.h"
#include "LauncherModel.h"

#include "unity-shared/Introspectable.h"
#include <sigc++/sigc++.h>

namespace unity
{
namespace switcher
{

/**
 * Provides a list of applications and application windows for the Switcher.
 *
 * This model provides a model two-level iterable data structure.  The first
 * level of data is effectively a list of applications available for selection.
 * Each application can further provide a second-level collection of windows for
 * selection.  To this end, the model provides the notion of the current
 * iterator value within each list, the second iterator value modally
 * corresponding to the current iterator value of the first.
 *
 * The mode of this model is changed by toggling the @p detail_selection
 * property.  Different iteration calls need to be made by client code depending
 * on the state of that property.
 */
class SwitcherModel : public debug::Introspectable, public sigc::trackable
{

public:
  typedef std::shared_ptr<SwitcherModel> Ptr;

  typedef std::vector<launcher::AbstractLauncherIcon::Ptr> Applications;
  typedef Applications::iterator iterator;
  typedef Applications::reverse_iterator reverse_iterator;

  nux::Property<bool>         detail_selection;
  nux::Property<unsigned int> detail_selection_index;
  nux::Property<bool>         only_detail_on_viewport;

  SwitcherModel(std::vector<launcher::AbstractLauncherIcon::Ptr> const& icons);
  virtual ~SwitcherModel();

  iterator begin();
  iterator end();

  reverse_iterator rbegin();
  reverse_iterator rend();

  launcher::AbstractLauncherIcon::Ptr at(unsigned int index) const;

  int Size() const;

  launcher::AbstractLauncherIcon::Ptr Selection() const;
  int SelectionIndex() const;
  bool SelectionIsActive() const;

  launcher::AbstractLauncherIcon::Ptr LastSelection() const;
  int LastSelectionIndex() const;

  std::vector<Window> DetailXids() const;
  Window DetailSelectionWindow() const;

  void Next();
  void Prev();

  void NextDetail();
  void PrevDetail();

  void NextDetailRow();
  void PrevDetailRow();
  bool HasNextDetailRow() const;
  bool HasPrevDetailRow() const;

  void SetRowSizes(std::vector<int> const& row_sizes);

  void Select(launcher::AbstractLauncherIcon::Ptr const& selection);
  void Select(unsigned int index);

  sigc::signal<void, launcher::AbstractLauncherIcon::Ptr const&> selection_changed;

protected:
  // Introspectable methods
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  void UpdateRowIndex();
  unsigned int SumNRows(unsigned int n) const;
  bool DetailIndexInLeftHalfOfRow() const;

  Applications                        applications_;
  unsigned int                        index_;
  unsigned int                        last_index_;
  unsigned int                        row_index_;
  launcher::AbstractLauncherIcon::Ptr last_active_application_;
  std::vector<int>                    row_sizes_;
};

}
}

#endif // SWITCHERMODEL_H

