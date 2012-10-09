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
bool compare_windows_by_active(Window first, Window second)
{
  WindowManager& wm = WindowManager::Default();
  return wm.GetWindowActiveNumber(first) > wm.GetWindowActiveNumber(second);
}

}


SwitcherModel::SwitcherModel(std::vector<AbstractLauncherIcon::Ptr> icons)
  : _inner(icons)
  , _index(0)
  , _last_index(0)
{
  detail_selection = false;
  detail_selection_index = 0;
  only_detail_on_viewport = false;

  for (auto icon : _inner)
  {
    AddChild(icon.GetPointer());
    if (icon->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE))
      _last_active_icon = icon;
  }
}

SwitcherModel::~SwitcherModel()
{
  for (auto icon : _inner)
  {
    RemoveChild(icon.GetPointer());
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

SwitcherModel::iterator
SwitcherModel::begin()
{
  return _inner.begin();
}

SwitcherModel::iterator
SwitcherModel::end()
{
  return _inner.end();
}

SwitcherModel::reverse_iterator
SwitcherModel::rbegin()
{
  return _inner.rbegin();
}

SwitcherModel::reverse_iterator
SwitcherModel::rend()
{
  return _inner.rend();
}

AbstractLauncherIcon::Ptr
SwitcherModel::at(unsigned int index)
{
  if ((int) index >= Size ())
    return AbstractLauncherIcon::Ptr();
  return _inner[index];
}

int
SwitcherModel::Size()
{
  return _inner.size();
}

AbstractLauncherIcon::Ptr
SwitcherModel::Selection()
{
  return _inner.at(_index);
}

int
SwitcherModel::SelectionIndex()
{
  return _index;
}

AbstractLauncherIcon::Ptr
SwitcherModel::LastSelection()
{
  return _inner.at(_last_index);
}

int SwitcherModel::LastSelectionIndex()
{
  return _last_index;
}

bool WindowOnOtherViewport(Window xid)
{
  return !WindowManager::Default().IsWindowOnCurrentDesktop(xid);
}

std::vector<Window> SwitcherModel::DetailXids()
{
  std::vector<Window> results;
  results = Selection()->Windows();

  if (only_detail_on_viewport)
  {
    results.erase(std::remove_if(results.begin(),
                                 results.end(),
                                 WindowOnOtherViewport),
                  results.end());
  }

  std::sort(results.begin(), results.end(), compare_windows_by_active);

  if (Selection() == _last_active_icon && results.size () > 1)
    for (unsigned int i = 0; i < results.size()-1; i++)
      std::swap (results[i], results[i+1]);

  return results;
}

Window SwitcherModel::DetailSelectionWindow()
{
  if (!detail_selection || DetailXids ().empty())
    return 0;

  if (detail_selection_index > DetailXids().size() - 1)
    return 0;

  return DetailXids()[detail_selection_index];
}

void SwitcherModel::Next()
{
  _last_index = _index;

  _index++;
  if (_index >= _inner.size())
    _index = 0;

  detail_selection = false;
  detail_selection_index = 0;
  selection_changed.emit(Selection());
}

void SwitcherModel::Prev()
{
  _last_index = _index;

  if (_index > 0)
    _index--;
  else
    _index = _inner.size() - 1;

  detail_selection = false;
  detail_selection_index = 0;
  selection_changed.emit(Selection());
}

void SwitcherModel::NextDetail ()
{
  if (!detail_selection())
    return;

  if (detail_selection_index < DetailXids().size() - 1)
    detail_selection_index = detail_selection_index + 1;
  else
    detail_selection_index = 0;
}

void SwitcherModel::PrevDetail ()
{
  if (!detail_selection())
    return;

  if (detail_selection_index >= (unsigned int) 1)
    detail_selection_index = detail_selection_index - 1;
  else
    detail_selection_index = DetailXids().size() - 1;
}

void SwitcherModel::Select(AbstractLauncherIcon::Ptr const& selection)
{
  int i = 0;
  for (iterator it = begin(), e = end(); it != e; ++it)
  {
    if (*it == selection)
    {
      if ((int) _index != i)
      {
        _last_index = _index;
        _index = i;

        detail_selection = false;
        detail_selection_index = 0;
        selection_changed.emit(Selection());
      }
      break;
    }
    ++i;
  }
}

void
SwitcherModel::Select(unsigned int index)
{
  unsigned int target = CLAMP(index, 0, _inner.size() - 1);

  if (target != _index)
  {
    _last_index = _index;
    _index = target;

    detail_selection = false;
    detail_selection_index = 0;
    selection_changed.emit(Selection());
  }
}

}
}
