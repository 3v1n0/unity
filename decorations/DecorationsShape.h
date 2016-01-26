#ifndef DECORATIONS_SHAPE_H_
#define DECORATIONS_SHAPE_H_

#include "WindowManager.h"
#include "DecoratedWindow.h"

class DecorationsShape
{
private:
  std::vector<XRectangle> rects;
  int width, height;

public:
  bool initShape(XID win);
  const XRectangle& getRectangle(int idx) const;
  int getRectangleCount() const;
  int getWidth() const;
  int getHeight() const;
  void clear();
};
#endif //DECORATIONS_SHAPE_H_
