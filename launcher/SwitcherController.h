// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2013 Canonical Ltd
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

#ifndef SWITCHERCONTROLLER_H
#define SWITCHERCONTROLLER_H

#include <memory>
#include <string>
#include <sigc++/connection.h>

#include "AbstractLauncherIcon.h"
#include <Nux/Nux.h>
#include "unity-shared/Introspectable.h"
#include "unity-shared/LayoutSystem.h"


namespace unity
{
namespace switcher
{

class SwitcherView;

enum class SortMode
{
  LAUNCHER_ORDER,
  FOCUS_ORDER,
};

enum class ShowMode
{
  ALL,
  CURRENT_VIEWPORT,
};

enum class DetailMode
{
  TAB_NEXT_WINDOW,
  TAB_NEXT_WINDOW_LOOP,
  TAB_NEXT_TILE,
};

enum class FirstSelectionMode
{
  LAST_ACTIVE_VIEW,
  LAST_ACTIVE_APP
};

/**
 * Represents a selected application+window to be switched to.
 */
struct Selection
{
  launcher::AbstractLauncherIcon::Ptr application_;
  Window                              window_;
};


class Controller : public debug::Introspectable, public sigc::trackable
{
public:
  class Impl;
  typedef std::function<nux::ObjectPtr<nux::BaseWindow>()> WindowCreator;
  typedef std::unique_ptr<Impl> ImplPtr;
  typedef std::shared_ptr<Controller> Ptr;

public:
  Controller(WindowCreator const& create_window = nullptr);

  ~Controller();

  void Show(ShowMode show,
            SortMode sort,
            std::vector<launcher::AbstractLauncherIcon::Ptr> const& results);
  void Hide(bool accept_state=true);

  bool CanShowSwitcher(std::vector<launcher::AbstractLauncherIcon::Ptr> const& resutls) const;
  void AddIcon(launcher::AbstractLauncherIcon::Ptr const&);
  void RemoveIcon(launcher::AbstractLauncherIcon::Ptr const&);

  bool Visible();
  nux::Geometry GetInputWindowGeometry() const;

  void Next();
  void Prev();

  void InitiateDetail();
  void NextDetail();
  void PrevDetail();

  void Select(int index);

  bool IsDetailViewShown();
  void SetDetail(bool detail,
                 unsigned int min_windows = 1);

  void SelectFirstItem();

  nux::ObjectPtr<SwitcherView> GetView() const;

  ui::LayoutWindow::Vector const& ExternalRenderTargets() const;

  int StartIndex() const;
  double Opacity() const;

  Selection GetCurrentSelection() const;

  sigc::connection ConnectToViewBuilt(sigc::slot<void> const&);

  // Introspectable methods
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

  nux::RWProperty<bool> detail;
  nux::ROProperty<DetailMode> detail_mode;
  nux::Property<FirstSelectionMode> first_selection_mode;
  nux::Property<bool> show_desktop_disabled;
  nux::Property<bool> mouse_disabled;
  nux::Property<int>  timeout_length;
  nux::Property<bool> detail_on_timeout;
  nux::Property<int>  detail_timeout_length;
  nux::Property<int>  initial_detail_timeout_length;

private:
  bool       visible_;
  int        monitor_;
  DetailMode detail_mode_;

  ImplPtr    impl_;
};

}
}

#endif // SWITCHERCONTROLLER_H

