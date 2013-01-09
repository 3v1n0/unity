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

#include <memory>

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

class Controller
{
  public:

    class Impl;
    typedef std::unique_ptr<Impl> ImplPtr;
    typedef std::function<ImplPtr()> CreateImplFunc;
    typedef std::shared_ptr<Controller> Ptr;

  public:

    Controller(CreateImplFunc const&);

    void Show(ShowMode show,
              SortMode sort,
              std::vector<launcher::AbstractLauncherIcon::Ptr> results);
    void Hide(bool accept_state=true);

    bool CanShowSwitcher(const std::vector<launcher::AbstractLauncherIcon::Ptr>& resutls) const;

    bool Visible();

    void Next();
    void Prev();

    void NextDetail();
    void PrevDetail();

    void Select(int index);

    void SetDetail(bool detail,
                   unsigned int min_windows = 1);

    void SelectFirstItem();

    void SetWorkspace(nux::Geometry geo,
                      int monitor);

    SwitcherView * GetView ();

    ui::LayoutWindow::Vector ExternalRenderTargets ();

    guint GetSwitcherInputWindowId() const;

    bool IsShowDesktopDisabled() const;
    void SetShowDesktopDisabled(bool disabled);
    int StartIndex() const;

    sigc::connection ConnectToViewBuilt (sigc::slot<void> const&);
    void SetDetailOnTimeout(bool timeout);

  private:
    ImplPtr impl_;
};

class Controller::Impl
{
  public:

    virtual ~Impl () {}

    virtual void Show(ShowMode show,
                      SortMode sort,
                      std::vector<launcher::AbstractLauncherIcon::Ptr> results) = 0;
    virtual void Hide(bool accept_state) = 0;

    virtual bool CanShowSwitcher(const std::vector<launcher::AbstractLauncherIcon::Ptr>& resutls) const = 0;

    virtual bool Visible() = 0;

    virtual void Next() = 0;
    virtual void Prev() = 0;

    virtual void NextDetail() = 0;
    virtual void PrevDetail() = 0;

    virtual void Select(int index) = 0;

    virtual void SetDetail(bool detail,
                           unsigned int min_windows) = 0;

    virtual void SelectFirstItem() = 0;

    virtual void SetWorkspace(nux::Geometry geo,
                              int monitor) = 0;

    virtual SwitcherView * GetView () = 0;

    virtual ui::LayoutWindow::Vector ExternalRenderTargets () = 0;

    virtual guint GetSwitcherInputWindowId() const = 0;

    virtual bool IsShowDesktopDisabled() const = 0;
    virtual void SetShowDesktopDisabled(bool disabled) = 0;
    virtual int StartIndex() const = 0;

    sigc::signal<void> view_built;
    nux::Property<bool> detail_on_timeout;
};

class ShellController : public Controller::Impl,
                        public debug::Introspectable,
                        public sigc::trackable
{
public:
  typedef std::shared_ptr<ShellController> Ptr;

  nux::Property<int> timeout_length;
  nux::Property<int>  detail_timeout_length;
  nux::Property<int> initial_detail_timeout_length;

  ShellController(unsigned int load_timeout = 20);

  virtual void Show(ShowMode show, SortMode sort, std::vector<launcher::AbstractLauncherIcon::Ptr> results);
  virtual void Hide(bool accept_state);

  bool CanShowSwitcher(const std::vector<launcher::AbstractLauncherIcon::Ptr>& resutls) const;

  virtual bool Visible();

  virtual void Next();
  virtual void Prev();

  void NextDetail();
  void PrevDetail();

  virtual void Select (int index);

  void SetDetail(bool detail, unsigned int min_windows = 1);

  void SelectFirstItem();

  void SetWorkspace(nux::Geometry geo, int monitor);

  virtual SwitcherView* GetView();

  ui::LayoutWindow::Vector ExternalRenderTargets ();

  guint GetSwitcherInputWindowId() const;

  bool IsShowDesktopDisabled() const;
  void SetShowDesktopDisabled(bool disabled);
  int StartIndex() const;

protected:
  // Introspectable methods
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  virtual void ConstructWindow();
  virtual void ConstructView();
  virtual void ShowView();

  virtual bool OnDetailTimer();
  void OnModelSelectionChanged(launcher::AbstractLauncherIcon::Ptr const& icon);

  unsigned int construct_timeout_;

private:
  enum DetailMode
  {
    TAB_NEXT_WINDOW,
    TAB_NEXT_WINDOW_LOOP,
    TAB_NEXT_TILE,
  };

  void OnBackgroundUpdate(GVariant* data);
  static bool CompareSwitcherItemsPriority(launcher::AbstractLauncherIcon::Ptr const& first, launcher::AbstractLauncherIcon::Ptr const& second);

  SwitcherModel::Ptr model_;
  SwitcherView::Ptr view_;

  nux::Geometry workarea_;
  nux::ObjectPtr<nux::BaseWindow> view_window_;
  nux::HLayout* main_layout_;

  int monitor_;
  bool visible_;
  bool show_desktop_disabled_;
  nux::Color bg_color_;
  DetailMode detail_mode_;

  launcher::AbstractLauncherIcon::Ptr last_active_selection_;

  UBusManager ubus_manager_;
  glib::SourceManager sources_;
};

}
}

#endif // SWITCHERCONTROLLER_H

