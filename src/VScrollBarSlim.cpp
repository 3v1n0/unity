#include "Nux/Nux.h"
#include "VScrollBarSlim.h"


VScrollBarSlim::VScrollBarSlim()
{

}

VScrollBarSlim::~VScrollBarSlim()
{

}

void VScrollBarSlim::Draw(nux::GraphicsContext& GfxContext, bool force_draw)
{
    nux::Geometry base = GetGeometry();
    GfxContext.PushClippingRectangle(base);
    
    gPainter.PaintBackground(GfxContext, base);

    base.OffsetPosition(3, 10);
    base.OffsetSize(-6, -2*10);
    
    gPainter.Paint2DQuadColor(GfxContext, base, nux::Color(0x99FFFFFF));

    gPainter.PaintShape(GfxContext, m_TopThumb->GetGeometry(), nux::Color(0x99FFFFFF), nux::eSCROLLBAR_TRIANGLE_UP);
    gPainter.PaintShape(GfxContext, m_BottomThumb->GetGeometry(), nux::Color(0x99FFFFFF), nux::eSCROLLBAR_TRIANGLE_DOWN);

    base = m_SlideBar->GetGeometry();
    base.OffsetPosition(3, 0);
    base.OffsetSize(-6, 0);

    gPainter.Paint2DQuadColor(GfxContext, base, nux::Color(0.2156 * m_color_factor, 0.2156 * m_color_factor, 0.2156 * m_color_factor, 1.0f));

    GfxContext.PopClippingRectangle();
}

