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

#ifndef SWITCHERCONTROLLER_H
#define SWITCHERCONTROLLER_H

#include <UnityCore/Variant.h>
#include <UnityCore/GLibSource.h>

#include "unity-shared/Introspectable.h"
#include "unity-shared/UBusWrapper.h"

#include "SwitcherModel.h"
#include "SwitcherView.h"

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/WindowCompositor.h>

namespace unity
{
namespace launcher
{
class AbstractLauncherIcon;
}
namespace switcher
{

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


class Controller : public debug::Introspectable, public sigc::trackable
{
public:
  typedef std::shared_ptr<Controller> Ptr;

  Controller(unsigned int load_timeout = 20);

  nux::Property<int> timeout_length;
  nux::Property<bool> detail_on_timeout;
  nux::Property<int>  detail_timeout_length;
  nux::Property<int> initial_detail_timeout_length;

  void Show(ShowMode show, SortMode sort, bool reverse, std::vector<launcher::AbstractLauncherIcon::Ptr> results);
  void Hide(bool accept_state=true);

  bool Visible();

  void Next();
  void Prev();

  void NextDetail();
  void PrevDetail();

  void Select (int index);

  void SetDetail(bool detail, unsigned int min_windows = 1);

  void SelectFirstItem();

  void SetWorkspace(nux::Geometry geo, int monitor);

  SwitcherView * GetView ();

  ui::LayoutWindowList ExternalRenderTargets ();

  guint GetSwitcherInputWindowId() const;

  sigc::signal<void> view_built;

protected:
  // Introspectable methods
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  virtual void ConstructWindow();
  virtual void ConstructView();
  virtual void ShowView();

  virtual bool OnDetailTimer();
  void OnModelSelectionChanged(launcher::AbstractLauncherIcon::Ptr icon);

  unsigned int construct_timeout_;

private:
  enum DetailMode
  {
    TAB_NEXT_WINDOW,
    TAB_NEXT_WINDOW_LOOP,
    TAB_NEXT_TILE,
  };

  void OnBackgroundUpdate(GVariant* data);
  static bool CompareSwitcherItemsPriority(launcher::AbstractLauncherIcon::Ptr first, launcher::AbstractLauncherIcon::Ptr second);

  SwitcherModel::Ptr model_;
  SwitcherView::Ptr view_;

  nux::Geometry workarea_;
  nux::ObjectPtr<nux::BaseWindow> view_window_;
  nux::HLayout* main_layout_;

  int monitor_;
  bool visible_;
  nux::Color bg_color_;
  DetailMode detail_mode_;

  UBusManager ubus_manager_;
  glib::SourceManager sources_;
};

}
}

#endif // SWITCHERCONTROLLER_H

