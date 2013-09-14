#ifndef NUX_MOCK_H
#define NUX_MOCK_H

#include <Nux/Nux.h>

namespace nux
{

class InputAreaMock : public Object
{
  public:
    void GestureEvent(const GestureEvent &event)
    {
    } 
};

class ViewMock : public InputAreaMock
{
  public:
    sigc::signal<void, int, int, unsigned long, unsigned long> mouse_down;
    sigc::signal<void, int, int, unsigned long, unsigned long> mouse_up;
    sigc::signal<void, int, int, int, int, unsigned long, unsigned long> mouse_drag;
};

} // namespace nux

#endif // NUX_MOCK_H
