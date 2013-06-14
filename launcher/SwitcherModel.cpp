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

#include <math.h>

#include "SwitcherModel.h"
#include "unity-shared/WindowManager.h"

#include <UnityCore/Variant.h>

namespace unity
{
using launcher::AbstractLauncherIcon;

namespace switcher
{


SwitcherModel::SwitcherModel(std::vector<AbstractLauncherIcon::Ptr> const& icons)
  : detail_selection(false)
  , detail_selection_index(0)
  , only_detail_on_viewport(false)
  , applications_(icons)
  , index_(0)
  , last_index_(0)
  , row_index_(0)
{
  // When using Webapps, there are more than one active icon, so let's just pick
  // up the first one found which is the web browser.
  bool found = false;

  for (auto const& application : applications_)
  {
    AddChild(application.GetPointer());
    if (application->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE) && !found)
    {
      last_active_application_ = application;
      found = true;
    }
  }
}

SwitcherModel::~SwitcherModel()
{
  for (auto const& application : applications_)
  {
    RemoveChild(application.GetPointer());
  }
}

std::string SwitcherModel::GetName() const
{
  return "SwitcherModel";
}

void SwitcherModel::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder)
  .add("detail-selection", detail_selection)
  .add("detail-selection-index", (int)detail_selection_index)
  .add("detail-current-count", DetailXids().size())
  .add("only-detail-on-viewport", only_detail_on_viewport)
  .add("selection-index", SelectionIndex())
  .add("last-selection-index", LastSelectionIndex());
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
  if ((int) index >= Size ())
    return AbstractLauncherIcon::Ptr();
  return applications_[index];
}

int SwitcherModel::Size() const
{
  return applications_.size();
}

AbstractLauncherIcon::Ptr SwitcherModel::Selection() const
{
  return applications_.at(index_);
}

int SwitcherModel::SelectionIndex() const
{
  return index_;
}

bool SwitcherModel::SelectionIsActive() const
{
  return Selection()->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE);
}

AbstractLauncherIcon::Ptr SwitcherModel::LastSelection() const
{
  return applications_.at(last_index_);
}

int SwitcherModel::LastSelectionIndex() const
{
  return last_index_;
}

std::vector<Window> SwitcherModel::DetailXids() const
{
  WindowManager& wm = WindowManager::Default();
  std::vector<Window> results;

  for (auto& window : Selection()->Windows())
  {
    Window xid = window->window_id();

    if (!only_detail_on_viewport || wm.IsWindowOnCurrentDesktop(xid))
      results.push_back(xid);
  }

  std::sort(results.begin(), results.end(), [&wm](Window first, Window second) {
      return wm.GetWindowActiveNumber(first) > wm.GetWindowActiveNumber(second);
  });

  if (Selection() == last_active_application_ && results.size() > 1)
  {
    results.push_back(results.front());
    results.erase(results.begin());
  }

  return results;
}

Window SwitcherModel::DetailSelectionWindow() const
{
  auto windows = DetailXids();
  if (!detail_selection || windows.empty())
    return 0;

  if (detail_selection_index > windows.size() - 1)
    return 0;

  return windows[detail_selection_index];
}

void SwitcherModel::Next()
{
  last_index_ = index_;

  index_++;
  if (index_ >= applications_.size())
    index_ = 0;

  detail_selection = false;
  detail_selection_index = 0;
  row_index_ = 0;
  selection_changed.emit(Selection());
}

void SwitcherModel::Prev()
{
  last_index_ = index_;

  if (index_ > 0)
    index_--;
  else
    index_ = applications_.size() - 1;

  detail_selection = false;
  detail_selection_index = 0;
  row_index_ = 0;
  selection_changed.emit(Selection());
}

void SwitcherModel::NextDetail()
{
  if (!detail_selection())
    return;

  if (detail_selection_index < DetailXids().size() - 1)
    detail_selection_index = detail_selection_index + 1;
  else
    detail_selection_index = 0;

  UpdateRowIndex();
}

void SwitcherModel::PrevDetail()
{
  if (!detail_selection())
    return;

  if (detail_selection_index >= 1)
    detail_selection_index = detail_selection_index - 1;
  else
    detail_selection_index = DetailXids().size() - 1;

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

    current_row++;
  }
}

unsigned int SwitcherModel::SumNRows(unsigned int n) const
{
  unsigned int total = 0;
  for (unsigned int i = 0; i <= n; i++)
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
  if (HasNextDetailRow())
  {
    unsigned int current_row = row_sizes_[row_index_];
    unsigned int next_row    = row_sizes_[row_index_ + 1];
    unsigned int increment   = current_row;

    if (!DetailIndexInLeftHalfOfRow())
      increment = next_row;

    detail_selection_index = detail_selection_index + increment;
    row_index_++;
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
    detail_selection_index = detail_selection_index - 1;
  }
}

bool SwitcherModel::HasNextDetailRow() const
{
  return (row_index_ < row_sizes_.size() - 1);
}

bool SwitcherModel::HasPrevDetailRow() const
{
  return (detail_selection_index > 0);
}

void SwitcherModel::SetRowSizes(std::vector<int> row_sizes)
{
  row_sizes_ = row_sizes;
}

void SwitcherModel::Select(AbstractLauncherIcon::Ptr const& selection)
{
  int i = 0;
  for (iterator it = begin(), e = end(); it != e; ++it)
  {
    if (*it == selection)
    {
      if ((int) index_ != i)
      {
        last_index_ = index_;
        index_ = i;

        detail_selection = false;
        detail_selection_index = 0;
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

    detail_selection = false;
    detail_selection_index = 0;
    selection_changed.emit(Selection());
  }
}

}
}
