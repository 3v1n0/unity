#ifndef SWITCHER_CONTROLLER_MOCK_H
#define SWITCHER_CONTROLLER_MOCK_H

#include "NuxMock.h"

namespace unity {
namespace switcher {

class SwitcherViewMock : public nux::ViewMock
{
public:
  int IconIndexAt(int x, int y)
  {
    return x*y;
  }
};

class ControllerMock
{
public:
  ControllerMock()
  {
    Reset();
  }

  void Reset()
  {
    is_visible_ = false;
    prev_count_ = 0;
    next_count_ = 0;
    index_selected_ = -1;
  }

  typedef std::shared_ptr<ControllerMock> Ptr;

  sigc::signal<void> view_built;

  void Next()
  {
    ++next_count_;
  }

  void Prev()
  {
    ++prev_count_;
  }

  void Select(int index)
  {
    index_selected_ = index;
  }

  bool Visible()
  {
    return is_visible_;
  }

  void Hide()
  {
    is_visible_ = false;
  }

  SwitcherViewMock *GetView()
  {
    return &view_;
  }

  bool is_visible_;
  SwitcherViewMock view_;
  int prev_count_;
  int next_count_;
  int index_selected_;
};

} // namespace switcher
} // namespace unity


#endif // SWITCHER_CONTROLLER_MOCK_H
