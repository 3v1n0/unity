#ifndef UNITYSHELL_MOCK_H
#define UNITYSHELL_MOCK_H

#include <compiz_mock/core/core.h>
#include <sigc++/sigc++.h>
#include <NuxMock.h>

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
    : launcher_(new nux::InputAreaMock)
  {
  }

  virtual ~UnityScreenMock()
  {
    launcher_->Dispose();
  }

  static UnityScreenMock *get(CompScreenMock *screen)
  {
    return static_cast<UnityScreenMock*>(screen);

  }

  nux::InputAreaMock *LauncherView()
  {
    return launcher_;
  }

  nux::InputAreaMock *launcher_;
};

} // namespace unity

#endif // UNITYSHELL_MOCK_H
