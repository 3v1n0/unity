#ifndef UNITYSHELL_MOCK_H
#define UNITYSHELL_MOCK_H

#include <compiz_mock/core/core.h>
#include <sigc++/sigc++.h>
#include <NuxMock.h>

#include "SwitcherControllerMock.h"


namespace unity
{

class UnityWindowMock : public CompWindowMock
{
  public:
  static UnityWindowMock *get(CompWindowMock *window)
  {
    return static_cast<UnityWindowMock*>(window);

  }

  sigc::signal<void> being_destroyed;
};

class UnityScreenMock : public CompScreenMock
{
  public:
  UnityScreenMock()
  {
    switcher_controller_ = std::make_shared<switcher::ControllerMock>();
    Reset();
  }

  void Reset()
  {
    SetUpAndShowSwitcher_count_ = 0;
    switcher_controller_->Reset();
  }

  virtual ~UnityScreenMock()
  {
  }

  static UnityScreenMock *get(CompScreenMock *screen)
  {
    return static_cast<UnityScreenMock*>(screen);

  }

  void SetUpAndShowSwitcher()
  {
    ++SetUpAndShowSwitcher_count_;
    switcher_controller_->is_visible_ = true;
  }

  switcher::ControllerMock::Ptr switcher_controller()
  {
    return switcher_controller_;
  }

  switcher::ControllerMock::Ptr switcher_controller_;
  int SetUpAndShowSwitcher_count_;
};

} // namespace unity

#endif // UNITYSHELL_MOCK_H
