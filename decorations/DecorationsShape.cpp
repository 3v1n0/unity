#include <string.h>
#include <X11/extensions/shape.h>
#include "DecoratedWindow.h"
#include "DecorationsShape.h"

bool DecorationsShape::initShape(XID win)
{
  Bool buse, cuse;
  int bx, by, cx, cy;
  unsigned int bw, bh, cw, ch;
  Display *dpy = screen->dpy();

  XShapeQueryExtents(dpy, win, &buse, &bx, &by, &bw, &bh, &cuse, &cx, &cy, &cw, &ch);

  int kind;
  if (cuse) {
    width = cw;
    height = ch;
    kind = ShapeClip;
  }
  else if (buse) {
    width = bw;
    height = bh;
    kind = ShapeBounding;
  }
  else {
    fprintf(stderr, "XShapeQueryExtend returned no extends.\n");
    return false;
  }

  int rect_count, rect_order;
  XRectangle *rectangles;
  if (!(rectangles = XShapeGetRectangles(dpy, win, kind, &rect_count, &rect_order))) {
    fprintf(stderr, "Failed to get shape rectangles\n");
    return false;
  }

  for (int i=0; i< rect_count; i++) {
    rects.push_back(rectangles[i]);
  }

  XFree(rectangles);
  return true;
}

const XRectangle& DecorationsShape::getRectangle(int idx) const
{
  return rects[idx];
}

int DecorationsShape::getRectangleCount() const
{
  return (int)rects.size();
}

int DecorationsShape::getWidth() const
{
  return width;
}

int DecorationsShape::getHeight() const
{
  return height;
}

void DecorationsShape::clear()
{
  width = height = 0;
  rects.clear();
}
