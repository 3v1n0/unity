// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Tim Penhey <tim.penhey@canonical.com>
 */

#ifndef UNITYSHARED_BAMF_APPLICATION_MANAGER_H
#define UNITYSHARED_BAMF_APPLICATION_MANAGER_H

#include <libbamf/libbamf.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>

#include "unity-shared/ApplicationManager.h"


namespace unity
{

class BamfApplicationView: public ApplicationWindow
{
public:
  explicit BamfApplicationView(glib::Object< ::BamfView> const& view);

  virtual std::string title() const;

protected:
  glib::Object< ::BamfView> bamf_view_;
};

class BamfApplicationWindow: public BamfApplicationView
{
public:
  explicit BamfApplicationWindow(glib::Object< ::BamfView> const& window);

  virtual Window window_id() const;
  virtual int monitor() const;

private:
  glib::Object< ::BamfWindow> bamf_window_;
};

class BamfApplicationTab: public BamfApplicationView
{
public:
  explicit BamfApplicationTab(glib::Object< ::BamfView> const& tab);

  virtual Window window_id() const;
  virtual int monitor() const;

private:
  glib::Object< ::BamfTab> bamf_tab_;
};


class BamfApplication : public Application
{
public:
  explicit BamfApplication(glib::Object< ::BamfApplication> const& app);
  ~BamfApplication();

  virtual std::string icon() const;
  virtual std::string title() const;

  virtual WindowList get_windows() const;

private: // Property getters and setters
  bool GetSeen() const;
  bool SetSeen(bool const& param);

  bool GetSticky() const;
  bool SetSticky(bool const& param);

  bool GetVisible() const;
  bool GetActive() const;
  bool GetRunning() const;
  bool GetUrgent() const;

private:
  glib::Object< ::BamfApplication> bamf_app_;
  glib::Object< ::BamfView> bamf_view_;
  glib::SignalManager signals_;
};

class BamfApplicationManager : public ApplicationManager
{
public:
  BamfApplicationManager();
  ~BamfApplicationManager();

  virtual ApplicationWindowPtr GetActiveWindow() const;

  virtual ApplicationPtr active_application() const;
  virtual ApplicationPtr GetApplicationForDesktopFile(std::string const& desktop_file) const;


  virtual ApplicationList running_applications() const;

private:
  void OnViewOpened(BamfMatcher* matcher, BamfView* view);

private:
  glib::Object< ::BamfMatcher> matcher_;
  glib::Signal<void, ::BamfMatcher*, ::BamfView*> view_opened_signal_;

};

}

#endif // UNITYSHARED_APPLICATION_MANAGER_H
