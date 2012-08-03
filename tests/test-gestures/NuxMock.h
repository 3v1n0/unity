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

} // namespace nux

#endif // NUX_MOCK_H
