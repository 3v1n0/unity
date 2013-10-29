/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Tim Penhey  <tim.penhey@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */
#ifndef TESTS_MOCK_APPLICATION_H
#define TESTS_MOCK_APPLICATION_H

#include <map>
#include <gmock/gmock.h>
#include <gio/gdesktopappinfo.h>
#include <UnityCore/GLibWrapper.h>

#include "unity-shared/ApplicationManager.h"
#include "unity-shared/WindowManager.h"

using namespace testing;

namespace testmocks
{
struct MockApplicationWindow : unity::ApplicationWindow
{
  typedef std::shared_ptr<MockApplicationWindow> Ptr;
  typedef NiceMock<MockApplicationWindow> Nice;

  MockApplicationWindow(Window xid)
    : xid_(xid)
    , monitor_(0)
    , title_("MockApplicationWindow "+std::to_string(xid_))
    , type_("window")
    , visible_(true)
    , active_(false)
    , urgent_(false)
  {
    visible.SetGetterFunction([this] { return visible_; });
    active.SetGetterFunction([this] { return active_; });
    urgent.SetGetterFunction([this] { return urgent_; });
    title.SetGetterFunction([this] { return title_; });
    icon.SetGetterFunction([this] { return icon_; });

    ON_CALL(*this, type()).WillByDefault(Invoke([this] { return type_; }));
    ON_CALL(*this, window_id()).WillByDefault(Invoke([this] { return xid_; }));
    ON_CALL(*this, monitor()).WillByDefault(Invoke([this] { return monitor_; }));
    ON_CALL(*this, Focus()).WillByDefault(Invoke([this] { return LocalFocus(); }));
    ON_CALL(*this, application()).WillByDefault(Return(unity::ApplicationPtr()));
  }

  Window xid_;
  int monitor_;
  std::string title_;
  std::string icon_;
  std::string type_;

  bool visible_;
  bool active_;
  bool urgent_;

  MOCK_CONST_METHOD0(type, std::string());
  MOCK_CONST_METHOD0(window_id, Window());
  MOCK_CONST_METHOD0(monitor, int());
  MOCK_CONST_METHOD0(application, unity::ApplicationPtr());
  MOCK_CONST_METHOD0(Focus, bool());
  MOCK_CONST_METHOD0(Quit, void());

  virtual bool LocalFocus() const
  {
    auto& wm = unity::WindowManager::Default();
    wm.Raise(xid_);
    wm.Activate(xid_);
    return true;
  }

  void SetTitle(std::string const& new_title)
  {
    if (new_title == title())
      return;

    title_ = new_title;
    title.changed(title_);
  }

  void SetIcon(std::string const& new_icon)
  {
    if (new_icon == icon())
      return;

    icon_ = new_icon;
    icon.changed(icon_);
  }
};

struct MockApplication : unity::Application
{
  typedef std::shared_ptr<MockApplication> Ptr;
  typedef NiceMock<MockApplication> Nice;

  MockApplication()
    : MockApplication("")
  {}

  MockApplication(std::string const& desktop_file_path,
                  std::string const& icon_name = "",
                  std::string const& title_str = "")
    : desktop_file_(desktop_file_path)
    , icon_(icon_name)
    , title_(title_str)
    , seen_(false)
    , sticky_(false)
    , visible_(false)
    , active_(false)
    , running_(false)
    , urgent_(false)
    , type_("mock")
    {
      seen.SetSetterFunction(sigc::mem_fun(this, &MockApplication::SetSeen));
      sticky.SetSetterFunction(sigc::mem_fun(this, &MockApplication::SetSticky));

      seen.SetGetterFunction([this] { return seen_; });
      sticky.SetGetterFunction([this] { return sticky_; });
      visible.SetGetterFunction([this] { return visible_; });
      active.SetGetterFunction([this] { return active_; });
      running.SetGetterFunction([this] { return running_; });
      urgent.SetGetterFunction([this] { return urgent_; });
      desktop_file.SetGetterFunction([this] { return desktop_file_; });
      title.SetGetterFunction([this] { return title_; });
      icon.SetGetterFunction([this] { return icon_; });

      ON_CALL(*this, type()).WillByDefault(Invoke([this] { return type_; }));
      ON_CALL(*this, desktop_id()).WillByDefault(Invoke([this] { return desktop_file_; }));
      ON_CALL(*this, repr()).WillByDefault(Return("MockApplication"));
      ON_CALL(*this, GetWindows()).WillByDefault(Invoke([this] { return windows_; }));
      ON_CALL(*this, GetSupportedMimeTypes()).WillByDefault(Return(std::vector<std::string>()));
      ON_CALL(*this, GetFocusableWindow()).WillByDefault(Return(unity::ApplicationWindowPtr()));
      ON_CALL(*this, OwnsWindow(_)).WillByDefault(Invoke(this, &MockApplication::LocalOwnsWindow));
      ON_CALL(*this, LogEvent(_,_)).WillByDefault(Invoke(this, &MockApplication::LocalLogEvent));
    }

  std::string desktop_file_;
  std::string icon_;
  std::string title_;
  bool seen_;
  bool sticky_;
  bool visible_;
  bool active_;
  bool running_;
  bool urgent_;
  unity::WindowList windows_;
  std::string type_;
  std::vector<std::pair<unity::ApplicationEventType, unity::ApplicationSubjectPtr>> actions_log_;

  MOCK_CONST_METHOD0(type, std::string());
  MOCK_CONST_METHOD0(repr, std::string());
  MOCK_CONST_METHOD0(desktop_id, std::string());
  MOCK_CONST_METHOD0(GetWindows, unity::WindowList());
  MOCK_CONST_METHOD1(OwnsWindow, bool(Window));
  MOCK_CONST_METHOD0(GetSupportedMimeTypes, std::vector<std::string>());
  MOCK_CONST_METHOD0(GetFocusableWindow, unity::ApplicationWindowPtr());
  MOCK_CONST_METHOD0(CreateLocalDesktopFile, bool());
  MOCK_CONST_METHOD2(LogEvent, void(unity::ApplicationEventType, unity::ApplicationSubjectPtr const&));
  MOCK_CONST_METHOD2(Focus, void(bool, int));
  MOCK_CONST_METHOD0(Quit, void());

  bool LocalOwnsWindow(Window window_id) const {
    auto end = std::end(windows_);
    return std::find_if(std::begin(windows_), end, [window_id] (unity::ApplicationWindowPtr window) {
      return window->window_id() == window_id;
    }) != end;
  }

  void LocalLogEvent(unity::ApplicationEventType type, unity::ApplicationSubjectPtr const& subject)
  {
    if (subject)
      actions_log_.push_back(std::make_pair(type, subject));
  }

  bool HasLoggedEvent(unity::ApplicationEventType type, unity::ApplicationSubjectPtr const& subject)
  {
    if (!subject)
      return false;

    for (auto const& pair : actions_log_)
    {
      if (pair.first == type && *pair.second == *subject)
        return true;
    }

    return false;
  }

  void SetRunState(bool state) {
    if (running_ == state)
      return;

    running_ = state;
    running.changed.emit(running_);
  }

  void SetVisibility(bool state) {
    if (visible_ == state)
      return;

    visible_ = state;
    visible.changed.emit(visible_);
  }

  bool SetSeen(bool const& param) {
    if (param != seen_) {
      seen_ = param;
      return true;
    }
    return false;
  }

  bool SetSticky(bool const& param) {
    if (param != sticky_) {
      sticky_ = param;
      return true;
    }
    return false;
  }

  void SetActiveState(bool state)
  {
    if (active_ == state)
      return;
    active_ = state;
    active.changed.emit(state);
  }

  void SetTitle(std::string const& new_title)
  {
    if (new_title == title())
      return;

    title_ = new_title;
    title.changed(title_);
  }

  void SetIcon(std::string const& new_icon)
  {
    if (new_icon == icon())
      return;

    icon_ = new_icon;
    icon.changed(icon_);
  }
};

struct MockApplicationSubject : unity::ApplicationSubject
{
  MockApplicationSubject()
  {
    uri.SetSetterFunction([this] (std::string const& val) { if (val == uri_) return false; uri_ = val; return true; });
    uri.SetGetterFunction([this] { return uri_; });
    origin.SetSetterFunction([this] (std::string const& val) { if (val == origin_) return false; origin_ = val; return true; });
    origin.SetGetterFunction([this] { return origin_; });
    text.SetSetterFunction([this] (std::string const& val) { if (val == text_) return false; text_ = val; return true; });
    text.SetGetterFunction([this] { return text_; });
    storage.SetSetterFunction([this] (std::string const& val) { if (val == storage_) return false; storage_ = val; return true; });
    storage.SetGetterFunction([this] { return storage_; });
    current_uri.SetSetterFunction([this] (std::string const& val) { if (val == current_uri_) return false; current_uri_ = val; return true; });
    current_uri.SetGetterFunction([this] { return current_uri_; });
    current_origin.SetSetterFunction([this] (std::string const& val) { if (val == current_origin_) return false; current_origin_ = val; return true; });
    current_origin.SetGetterFunction([this] { return current_origin_; });
    mimetype.SetSetterFunction([this] (std::string const& val) { if (val == mimetype_) return false; mimetype_ = val; return true; });
    mimetype.SetGetterFunction([this] { return mimetype_; });
    interpretation.SetSetterFunction([this] (std::string const& val) { if (val == interpretation_) return false; interpretation_ = val; return true; });
    interpretation.SetGetterFunction([this] { return interpretation_; });
    manifestation.SetSetterFunction([this] (std::string const& val) { if (val == manifestation_) return false; manifestation_ = val; return true; });
    manifestation.SetGetterFunction([this] { return manifestation_; });
  }

  std::string uri_;
  std::string origin_;
  std::string text_;
  std::string storage_;
  std::string current_uri_;
  std::string current_origin_;
  std::string mimetype_;
  std::string interpretation_;
  std::string manifestation_;
};

struct MockApplicationManager : public unity::ApplicationManager
{
  typedef NiceMock<MockApplicationManager> Nice;

  MockApplicationManager()
  {
    ON_CALL(*this, GetUnityApplication()).WillByDefault(Invoke(this, &MockApplicationManager::LocalGetUnityApplication));
    ON_CALL(*this, GetApplicationForDesktopFile(_)).WillByDefault(Invoke(this, &MockApplicationManager::LocalGetApplicationForDesktopFile));
    ON_CALL(*this, GetActiveWindow()).WillByDefault(Invoke([this] { return unity::ApplicationWindowPtr(); } ));
    ON_CALL(*this, GetRunningApplications()).WillByDefault(Invoke([this] { return unity::ApplicationList(); } ));
    ON_CALL(*this, GetApplicationForWindow(_)).WillByDefault(Invoke([this] (Window) { return unity::ApplicationPtr(); } ));
  }

  static void StartApp(std::string const& desktop_file)
  {
    // We know the application manager is a mock one so we can cast it.
    auto& app_manager = unity::ApplicationManager::Default();
    auto self = dynamic_cast<MockApplicationManager*>(&app_manager);
    auto app = self->LocalGetApplicationForDesktopFile(desktop_file);
    app_manager.application_started.emit(app);
  }

  MOCK_CONST_METHOD0(GetUnityApplication, unity::ApplicationPtr());
  MOCK_CONST_METHOD0(GetActiveWindow, unity::ApplicationWindowPtr());
  MOCK_CONST_METHOD1(GetApplicationForDesktopFile, unity::ApplicationPtr(std::string const&));
  MOCK_CONST_METHOD0(GetRunningApplications, unity::ApplicationList());
  MOCK_CONST_METHOD1(GetApplicationForWindow, unity::ApplicationPtr(Window));

  unity::ApplicationPtr LocalGetApplicationForDesktopFile(std::string const& desktop_file)
  {
    AppMap::iterator iter = app_map_.find(desktop_file);
    if (iter == app_map_.end())
    {
      std::string title;
      std::string icon;
      std::shared_ptr<GKeyFile> key_file(g_key_file_new(), g_key_file_free);

      if (g_key_file_load_from_file(key_file.get(), desktop_file.c_str(), G_KEY_FILE_NONE, nullptr))
      {
        title = unity::glib::String(g_key_file_get_string(key_file.get(), G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NAME, nullptr)).Str();
        icon = unity::glib::String(g_key_file_get_string(key_file.get(), G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_ICON, nullptr)).Str();
      }

      auto app = std::make_shared<MockApplication::Nice>(desktop_file, icon, title);
      app_map_.insert(AppMap::value_type(desktop_file, app));
      return app;
    }
    else
    {
      return iter->second;
    }
  }

  unity::ApplicationPtr LocalGetUnityApplication() const
  {
    static unity::ApplicationPtr unity(new MockApplication::Nice);
    auto unity_mock = std::static_pointer_cast<MockApplication>(unity);
    unity_mock->desktop_file_ = "compiz.desktop";
    unity_mock->title_ = "Unity Desktop";
    unity_mock->running_ = true;

    return unity;
  }

private:
  typedef std::map<std::string, unity::ApplicationPtr> AppMap;
  AppMap app_map_;
};

struct TestUnityAppBase : testing::Test
{
  TestUnityAppBase()
  {
    auto const& unity_app = unity::ApplicationManager::Default().GetUnityApplication();
    unity_app_ = std::static_pointer_cast<MockApplication>(unity_app);
  }

  ~TestUnityAppBase()
  {
    Mock::VerifyAndClearExpectations(unity_app_.get());
    unity_app_->actions_log_.clear();
  }

  MockApplication::Ptr unity_app_;
};

}

#endif
