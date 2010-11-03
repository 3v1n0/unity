#ifndef VSCROLLBARSLIM_H
#define VSCROLLBARSLIM_H

#include "Nux/VScrollBar.h"

class VScrollBarSlim : public nux::VScrollBar
{
public:
    VScrollBarSlim();
    ~VScrollBarSlim();
    virtual void Draw(nux::GraphicsContext& GfxContext, bool force_draw);
};

#endif // VSCROLLBARSLIM_H
