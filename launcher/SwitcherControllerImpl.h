/*
 * Copyright (C) 2013 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License version 3, as published 
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranties of 
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along 
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SWITCHERCONTROLLERIMPL_H
#define SWITCHERCONTROLLERIMPL_H

#include <memory>

#include "AbstractLauncherIcon.h"
#include <UnityCore/Variant.h>
#include <UnityCore/GLibSource.h>

#include "unity-shared/Introspectable.h"
#include "unity-shared/MockableBaseWindow.h"
#include "unity-shared/UBusWrapper.h"

#include "SwitcherModel.h"
#include "SwitcherView.h"

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/WindowCompositor.h>
#include <NuxCore/Animation.h>

namespace unity
{
namespace switcher
{

struct Controller::Impl : public sigc::trackable
{
  Impl(Controller* obj,
       unsigned int load_timeout,
       Controller::WindowCreator const& create_window);

  void Show(ShowMode show, SortMode sort, std::vector<launcher::AbstractLauncherIcon::Ptr> results);
  void Hide(bool accept_state);
  void DetailHide();

  void Next();
  void Prev();

  void InitiateDetail(bool animate=false);
  void NextDetail();
  void PrevDetail();

  void NextDetailRow();
  void PrevDetailRow();
  bool HasNextDetailRow() const;
  bool HasPrevDetailRow() const;

  bool IsDetailViewShown();
  void SetDetail(bool detail, unsigned int min_windows = 1);

  void SelectFirstItem();

  virtual SwitcherView::Ptr GetView() const;

  ui::LayoutWindow::Vector ExternalRenderTargets();

  guint GetSwitcherInputWindowId() const;

  int StartIndex() const;
  Selection GetCurrentSelection() const;

  sigc::signal<void> view_built;

  void ConstructWindow();
  void ConstructView();
  void ShowView();
  void HideWindow();

  void ResetDetailTimer(int timeout_length);
  bool OnDetailTimer();
  void OnModelSelectionChanged(launcher::AbstractLauncherIcon::Ptr const& icon);
  void OnBackgroundUpdate(nux::Color const&);

  unsigned int construct_timeout_;

  Controller* obj_;

  SwitcherModel::Ptr model_;
  SwitcherView::Ptr view_;

  // @todo move these view data into the SwitcherView class
  Controller::WindowCreator create_window_;
  MockableBaseWindow::Ptr view_window_;
  nux::HLayout* main_layout_;
  nux::animation::AnimateValue<double> fade_animator_;

  UBusManager ubus_manager_;
  glib::SourceManager sources_;
};

}
}

#endif // SWITCHERCONTROLLERIMPL_H

