/*
 * Copyright (C) 2011-2015 Canonical Ltd
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
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <math.h>

#include "SwitcherModel.h"
#include "unity-shared/WindowManager.h"
#include <UnityCore/Variant.h>

namespace unity
{
using launcher::AbstractLauncherIcon;

namespace switcher
{
namespace
{
/**
 * Helper comparison functor for sorting application icons.
 */
bool CompareSwitcherItemsPriority(AbstractLauncherIcon::Ptr const& first,
                                  AbstractLauncherIcon::Ptr const& second)
{
  return first->SwitcherPriority() > second->SwitcherPriority();
}
}


SwitcherModel::SwitcherModel(Applications const& icons, bool sort_by_priority)
  : detail_selection(false)
  , detail_selection_index(0)
  , only_apps_on_viewport(true)
  , applications_(icons)
  , sort_by_priority_(sort_by_priority)
  , index_(0)
  , last_index_(0)
  , row_index_(0)
{
  for (auto it = applications_.begin(); it != applications_.end();)
  {
    ConnectToIconSignals(*it);

    if (!(*it)->ShowInSwitcher(only_apps_on_viewport))
    {
      hidden_applications_.push_back(*it);
      it = applications_.erase(it);
      continue;
    }

    ++it;
  }

  if (sort_by_priority_)
    std::sort(std::begin(applications_), std::end(applications_), CompareSwitcherItemsPriority);

  UpdateLastActiveApplication();

  only_apps_on_viewport.changed.connect([this] (bool) {
    VerifyApplications();
  });

  detail_selection.changed.connect([this] (bool) {
    UpdateDetailXids();
  });
}

void SwitcherModel::UpdateLastActiveApplication()
{
  for (auto const& application : applications_)
  {
    if (application->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE))
    {
      last_active_application_ = application;
      break;
    }
  }
}

void SwitcherModel::VerifyApplications()
{
  bool anything_changed = false;

  for (auto it = applications_.begin(); it != applications_.end();)
  {
    if (!(*it)->ShowInSwitcher(only_apps_on_viewport))
    {
      unsigned icon_index = it - applications_.begin();
      hidden_applications_.push_back(*it);
      it = applications_.erase(it);
      anything_changed = true;
      bool was_in_detail = (detail_selection && icon_index == index_);

      if (icon_index < index_ || index_ == applications_.size())
        PrevIndex();

      if (was_in_detail)
        UnsetDetailSelection();

      continue;
    }

    ++it;
  }

  for (auto it = hidden_applications_.begin(); it != hidden_applications_.end();)
  {
    if ((*it)->ShowInSwitcher(only_apps_on_viewport))
    {
      InsertIcon(*it);
      it = hidden_applications_.erase(it);
      anything_changed = true;
      continue;
    }

    ++it;
  }

  if (anything_changed)
  {
    if (!last_active_application_ || !last_active_application_->ShowInSwitcher(only_apps_on_viewport))
      UpdateLastActiveApplication();

    updated.emit();
  }
}

void SwitcherModel::InsertIcon(AbstractLauncherIcon::Ptr const& application)
{
  if (sort_by_priority_)
  {
    auto pos = std::upper_bound(applications_.begin(), applications_.end(), application, CompareSwitcherItemsPriority);
    unsigned icon_index = pos - applications_.begin();
    applications_.insert(pos, application);

    if (icon_index <= index_)
      NextIndex();
  }
  else
  {
    applications_.push_back(application);
  }
}

void SwitcherModel::ConnectToIconSignals(launcher::AbstractLauncherIcon::Ptr const& icon)
{
  icon->quirks_changed.connect(sigc::hide(sigc::hide(sigc::mem_fun(this, &SwitcherModel::OnIconQuirksChanged))));
  icon->windows_changed.connect(sigc::hide(sigc::bind(sigc::mem_fun(this, &SwitcherModel::OnIconWindowsUpdated), icon.GetPointer())));
}

void SwitcherModel::AddIcon(AbstractLauncherIcon::Ptr const& icon)
{
  if (!icon)
    return;

  if (icon->ShowInSwitcher(only_apps_on_viewport))
  {
    if (icon->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE))
      last_active_application_ = icon;

    if (std::find(applications_.begin(), applications_.end(), icon) == applications_.end())
    {
      InsertIcon(icon);
      ConnectToIconSignals(icon);
      updated.emit();
    }
  }
  else if (std::find(hidden_applications_.begin(), hidden_applications_.end(), icon) == hidden_applications_.end())
  {
    hidden_applications_.push_back(icon);
    ConnectToIconSignals(icon);
  }
}

void SwitcherModel::RemoveIcon(launcher::AbstractLauncherIcon::Ptr const& icon)
{
  auto icon_it = std::find(applications_.begin(), applications_.end(), icon);
  if (icon_it != applications_.end())
  {
    unsigned icon_index = icon_it - applications_.begin();
    bool was_in_detail = (detail_selection && icon_index == index_);
    applications_.erase(icon_it);

    if (last_active_application_ == icon)
      UpdateLastActiveApplication();

    if (icon_index < index_ || index_ == applications_.size())
      PrevIndex();

    if (was_in_detail)
      UnsetDetailSelection();

    updated.emit();
  }
  else
  {
    hidden_applications_.erase(std::remove(hidden_applications_.begin(), hidden_applications_.end(), icon), hidden_applications_.end());
  }
}

void SwitcherModel::OnIconQuirksChanged()
{
  auto old_selection = Selection();
  VerifyApplications();

  if (old_selection == last_active_application_)
    UpdateLastActiveApplication();

  auto const& new_selection = Selection();

  if (old_selection != new_selection)
    selection_changed.emit(new_selection);
}

void SwitcherModel::OnIconWindowsUpdated(AbstractLauncherIcon* icon)
{
  if (detail_selection() && icon == Selection().GetPointer())
  {
    UpdateDetailXids();

    if (detail_selection_index() >= detail_xids_.size())
      detail_selection_index = detail_xids_.empty() ? 0 : detail_xids_.size() - 1;
  }

  updated.emit();
}

std::string SwitcherModel::GetName() const
{
  return "SwitcherModel";
}

void SwitcherModel::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
  .add("detail-selection", detail_selection)
  .add("detail-selection-index", detail_selection_index())
  .add("detail-current-count", SelectionWindows().size())
  .add("detail-windows", glib::Variant::FromVector(SelectionWindows()))
  .add("only-apps-on-viewport", only_apps_on_viewport())
  .add("selection-index", SelectionIndex())
  .add("last-selection-index", LastSelectionIndex());
}

debug::Introspectable::IntrospectableList SwitcherModel::GetIntrospectableChildren()
{
  debug::Introspectable::IntrospectableList children;
  unsigned order = 0;

  for (auto const& icon : applications_)
  {
    if (!icon->ShowInSwitcher(only_apps_on_viewport))
    {
      icon->SetOrder(++order);
      children.push_back(icon.GetPointer());
    }
  }

  return children;
}

SwitcherModel::iterator SwitcherModel::begin()
{
  return applications_.begin();
}

SwitcherModel::iterator SwitcherModel::end()
{
  return applications_.end();
}

SwitcherModel::reverse_iterator SwitcherModel::rbegin()
{
  return applications_.rbegin();
}

SwitcherModel::reverse_iterator SwitcherModel::rend()
{
  return applications_.rend();
}

AbstractLauncherIcon::Ptr SwitcherModel::at(unsigned int index) const
{
  if (index >= applications_.size())
    return AbstractLauncherIcon::Ptr();

  return applications_[index];
}

size_t SwitcherModel::Size() const
{
  return applications_.size();
}

AbstractLauncherIcon::Ptr SwitcherModel::Selection() const
{
  return index_ < applications_.size() ? applications_.at(index_) : AbstractLauncherIcon::Ptr();
}

int SwitcherModel::SelectionIndex() const
{
  return index_;
}

bool SwitcherModel::SelectionIsActive() const
{
  auto const& selection = Selection();
  return selection ? selection->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE) : false;
}

AbstractLauncherIcon::Ptr SwitcherModel::LastSelection() const
{
  return applications_.at(last_index_);
}

int SwitcherModel::LastSelectionIndex() const
{
  return last_index_;
}

std::vector<Window> const& SwitcherModel::DetailXids() const
{
  return detail_xids_;
}

std::vector<Window> SwitcherModel::SelectionWindows() const
{
  if (!detail_xids_.empty())
    return detail_xids_;

  WindowManager& wm = WindowManager::Default();
  std::vector<Window> results;
  auto const& selection = Selection();

  if (!selection)
    return results;

  for (auto& window : selection->Windows())
  {
    Window xid = window->window_id();

    if (!only_apps_on_viewport || wm.IsWindowOnCurrentDesktop(xid))
      results.push_back(xid);
  }

  if (results.empty())
    return results;

  std::sort(results.begin(), results.end(), [&wm](Window first, Window second) {
    return wm.GetWindowActiveNumber(first) > wm.GetWindowActiveNumber(second);
  });

  if (selection == last_active_application_)
  {
    results.push_back(results.front());
    results.erase(results.begin());
  }

  return results;
}

void SwitcherModel::UpdateDetailXids()
{
  detail_xids_.clear();

  if (detail_selection)
    detail_xids_ = SelectionWindows();
}

Window SwitcherModel::DetailSelectionWindow() const
{
  if (!detail_selection || detail_xids_.empty())
    return 0;

  if (detail_selection_index > detail_xids_.size() - 1)
    return 0;

  return detail_xids_[detail_selection_index];
}

void SwitcherModel::UnsetDetailSelection()
{
  detail_selection = false;
  detail_selection_index = 0;
  row_index_ = 0;
}

void SwitcherModel::NextIndex()
{
  if (applications_.empty())
    return;

  last_index_ = index_;
  ++index_ %= applications_.size();
}

void SwitcherModel::Next()
{
  NextIndex();
  UnsetDetailSelection();
  selection_changed.emit(Selection());
}

void SwitcherModel::PrevIndex()
{
  if (applications_.empty())
    return;

  last_index_ = index_;
  index_ = ((index_ > 0 && index_ < applications_.size()) ? index_ : applications_.size()) - 1;
}

void SwitcherModel::Prev()
{
  PrevIndex();
  UnsetDetailSelection();
  selection_changed.emit(Selection());
}

void SwitcherModel::NextDetail()
{
  if (!detail_selection() || detail_xids_.empty())
    return;

  detail_selection_index = (detail_selection_index + 1) % detail_xids_.size();
  UpdateRowIndex();
}

void SwitcherModel::PrevDetail()
{
  if (!detail_selection() || detail_xids_.empty())
    return;

  detail_selection_index = ((detail_selection_index() > 0) ? detail_selection_index : detail_xids_.size()) - 1;
  UpdateRowIndex();
}

void SwitcherModel::UpdateRowIndex()
{
  int current_index = detail_selection_index;
  unsigned int current_row = 0;

  for (auto r : row_sizes_)
  {
    current_index -= r;

    if (current_index < 0)
    {
      row_index_ = current_row;
      return;
    }

    ++current_row;
  }
}

unsigned int SwitcherModel::SumNRows(unsigned int n) const
{
  unsigned int total = 0;

  if (n < row_sizes_.size())
    for (unsigned int i = 0; i <= n; ++i)
      total += row_sizes_[i];

  return total;
}

bool SwitcherModel::DetailIndexInLeftHalfOfRow() const
{
  unsigned int half = row_sizes_[row_index_]/2;
  unsigned int total_above = (row_index_ > 0 ? SumNRows(row_index_ - 1) : 0);
  unsigned int diff = detail_selection_index - total_above;

  return (diff < half);
}

void SwitcherModel::NextDetailRow()
{
  if (row_sizes_.size() && row_index_ < row_sizes_.size() - 1)
  {
    unsigned int current_row = row_sizes_[row_index_];
    unsigned int next_row    = row_sizes_[row_index_ + 1];
    unsigned int increment   = current_row;

    if (!DetailIndexInLeftHalfOfRow())
      increment = next_row;

    detail_selection_index = detail_selection_index + increment;
    ++row_index_;
  }
  else
  {
    detail_selection_index = (detail_selection_index + 1) % detail_xids_.size();
  }
}

void SwitcherModel::PrevDetailRow()
{
  if (row_index_ > 0)
  {
    unsigned int current_row = row_sizes_[row_index_];
    unsigned int prev_row    = row_sizes_[row_index_ - 1];
    unsigned int decrement   = current_row;

    if (DetailIndexInLeftHalfOfRow())
      decrement = prev_row;

    detail_selection_index = detail_selection_index - decrement;
    row_index_--;
  }
  else
  {
    detail_selection_index = ((detail_selection_index() > 0) ? detail_selection_index : detail_xids_.size()) - 1;
  }
}

bool SwitcherModel::HasNextDetailRow() const
{
  return (detail_selection_index() < detail_xids_.size() - 1);
}

bool SwitcherModel::HasPrevDetailRow() const
{
  return (detail_selection_index() > 0);
}

void SwitcherModel::SetRowSizes(std::vector<int> const& row_sizes)
{
  row_sizes_ = row_sizes;
}

void SwitcherModel::Select(AbstractLauncherIcon::Ptr const& selection)
{
  unsigned i = 0;
  for (iterator it = begin(), e = end(); it != e; ++it)
  {
    if (*it == selection)
    {
      if (index_ != i)
      {
        last_index_ = index_;
        index_ = i;

        UnsetDetailSelection();
        selection_changed.emit(Selection());
      }
      break;
    }
    ++i;
  }
}

void SwitcherModel::Select(unsigned int index)
{
  unsigned int target = CLAMP(index, 0, applications_.size() - 1);

  if (target != index_)
  {
    last_index_ = index_;
    index_ = target;

    UnsetDetailSelection();
    selection_changed.emit(Selection());
  }
}

}
}
