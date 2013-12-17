// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <NuxCore/Logger.h>
#include "DecorationsPriv.h"

namespace unity
{
namespace decoration
{

namespace
{
DECLARE_LOGGER(logger, "unity.decoration.window");
const unsigned GRAB_BORDER = 10;
}

class Window::Impl::WindowButton : public TexturedItem
{
public:
  WindowButton(WindowButtonType type)
  {
    texture_.SetTexture(manager_->impl_->GetButtonTexture(type, mouse_owner() ? WidgetState::PRELIGHT : WidgetState::NORMAL));
    mouse_owner.changed.connect([this, type] (bool v) {
      texture_.SetTexture(manager_->impl_->GetButtonTexture(type, v ? WidgetState::PRELIGHT : WidgetState::NORMAL));
      Damage();
    });
  }
};

Window::Impl::Impl(Window* parent, CompWindow* win)
  : active(false)
  , parent_(parent)
  , win_(win)
  , cwin_(CompositeWindow::get(win_))
  , glwin_(GLWindow::get(win_))
  , frame_(0)
  , dirty_geo_(true)
{
  active.changed.connect([this] (bool) {
    bg_textures_.clear();
    parent_->UpdateDecorationPositionDelayed();
    cwin_->damageOutputExtents();
  });

  if (win_->isViewable() || win_->shaded())
    Update();
}

Window::Impl::~Impl()
{
  Undecorate();
}

void Window::Impl::Update()
{
  if (ShouldBeDecorated())
  {
    SetupExtents();
    UpdateFrame();
    SetupTopLayout();
  }
  else
  {
    Undecorate();
  }
}

void Window::Impl::Undecorate()
{
  UnsetExtents();
  UnsetFrame();
  top_layout_.reset();
  bg_textures_.clear();
}

void Window::Impl::UnsetExtents()
{
  if (win_->hasUnmapReference())
    return;

  CompWindowExtents empty(0, 0, 0, 0);

  if (win_->border() != empty || win_->input() != empty)
    win_->setWindowFrameExtents(&empty, &empty);
}

void Window::Impl::SetupExtents()
{
  if (win_->hasUnmapReference())
    return;

  auto const& style = Style::Get();
  CompWindowExtents border(style->BorderWidth(Side::LEFT),
                           style->BorderWidth(Side::RIGHT),
                           style->BorderWidth(Side::TOP),
                           style->BorderWidth(Side::BOTTOM));

  CompWindowExtents input(border);
  input.left += GRAB_BORDER;
  input.right += GRAB_BORDER;
  input.top += GRAB_BORDER;
  input.bottom += GRAB_BORDER;

  if (win_->border() != border || win_->input() != input)
    win_->setWindowFrameExtents(&border, &input);
}

void Window::Impl::UnsetFrame()
{
  if (!frame_)
    return;

  XDestroyWindow(dpy, frame_);
  frame_ = 0;
  frame_geo_.Set(0, 0, 0, 0);
}

void Window::Impl::UpdateFrame()
{
  auto const& input = win_->input();
  auto const& server = win_->serverGeometry();
  nux::Geometry frame_geo(0, 0, server.widthIncBorders() + input.left + input.right,
                          server.heightIncBorders() + input.top  + input.bottom);

  if (win_->shaded())
    frame_geo.height = input.top + input.bottom;

  if (!frame_)
  {
    /* Since we're reparenting windows here, we need to grab the server
     * which sucks, but its necessary */
    XGrabServer(dpy);

    XSetWindowAttributes attr;
    attr.event_mask = StructureNotifyMask;
    // attr.event_mask = ButtonPressMask | EnterWindowMask | LeaveWindowMask | ExposureMask;
    attr.override_redirect = True;

    auto parent = win_->frame();
    frame_ = XCreateWindow(dpy, parent, frame_geo.x, frame_geo.y,
                           frame_geo.width, frame_geo.height, 0, CopyFromParent,
                           InputOnly, CopyFromParent, CWOverrideRedirect | CWEventMask, &attr);

    if (screen->XShape())
      XShapeSelectInput(dpy, frame_, ShapeNotifyMask);

    XMapWindow(dpy, frame_);

    XGrabButton(dpy, AnyButton, AnyModifier, frame_, True,
                ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeSync,
                GrabModeSync, None, None);

    XSelectInput(dpy, frame_, ButtonPressMask | ButtonReleaseMask | EnterWindowMask |
                              LeaveWindowMask | PointerMotionMask | ButtonMotionMask |
                              PropertyChangeMask /*| FocusChangeMask*/);

    XUngrabServer(dpy);
  }

  if (frame_geo_ != frame_geo)
  {
    XMoveResizeWindow(dpy, frame_, frame_geo.x, frame_geo.y, frame_geo.width, frame_geo.height);
    XLowerWindow(dpy, frame_);

    int i = 0;
    XRectangle rects[4];

    rects[i].x = 0;
    rects[i].y = 0;
    rects[i].width = frame_geo.width;
    rects[i].height = input.top;

    if (rects[i].width && rects[i].height)
      i++;

    rects[i].x = 0;
    rects[i].y = input.top;
    rects[i].width = input.left;
    rects[i].height = frame_geo.height - input.top - input.bottom;

    if (rects[i].width && rects[i].height)
      i++;

    rects[i].x = frame_geo.width - input.right;
    rects[i].y = input.top;
    rects[i].width = input.right;
    rects[i].height = frame_geo.height - input.top - input.bottom;

    if (rects[i].width && rects[i].height)
      i++;

    rects[i].x = 0;
    rects[i].y = frame_geo.height - input.bottom;
    rects[i].width = frame_geo.width;
    rects[i].height = input.bottom;

    if (rects[i].width && rects[i].height)
      i++;

    XShapeCombineRectangles(dpy, frame_, ShapeBounding, 0, 0, rects, i, ShapeSet, YXBanded);

    frame_geo_ = frame_geo;
    SyncXShapeWithFrameRegion();
  }
}

void Window::Impl::SyncXShapeWithFrameRegion()
{
  frame_region_ = CompRegion();

  int n = 0;
  int order = 0;
  XRectangle *rects = NULL;

  rects = XShapeGetRectangles(dpy, frame_, ShapeInput, &n, &order);
  if (!rects)
    return;

  for (int i = 0; i < n; ++i)
  {
    auto& rect = rects[i];
    frame_region_ += CompRegion(rect.x, rect.y, rect.width, rect.height);
  }

  XFree(rects);

  win_->updateFrameRegion();
}

void Window::Impl::SetupTopLayout()
{
  if (top_layout_)
    return;

  top_area_ = std::make_shared<SimpleItem>();
  input_mixer_ = std::make_shared<InputMixer>();
  input_mixer_->PushToFront(top_area_);

  auto padding = Style::Get()->Padding(Side::TOP);
  top_layout_ = std::make_shared<Layout>();
  top_layout_->left_padding = padding.left;
  top_layout_->right_padding = padding.right;
  top_layout_->top_padding = padding.top;
  top_layout_->Append(std::make_shared<WindowButton>(WindowButtonType::CLOSE));
  top_layout_->Append(std::make_shared<WindowButton>(WindowButtonType::MINIMIZE));
  top_layout_->Append(std::make_shared<WindowButton>(WindowButtonType::MAXIMIZE));

  input_mixer_->PushToFront(top_layout_);
}

bool Window::Impl::ShadowDecorated() const
{
  if (!win_->isViewable())
    return false;

  if ((win_->state() & MAXIMIZE_STATE) == MAXIMIZE_STATE)
    return false;

  if (win_->wmType() & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
    return false;

  if (win_->region().numRects() != 1) // Non rectangular windows
    return false;

  if (win_->overrideRedirect() && win_->alpha())
    return false;

  return true;
}

bool Window::Impl::FullyDecorated() const
{
  if (!ShadowDecorated())
    return false;

  if (win_->overrideRedirect())
    return false;

  switch (win_->type())
  {
    case CompWindowTypeDialogMask:
    case CompWindowTypeModalDialogMask:
    case CompWindowTypeUtilMask:
    case CompWindowTypeMenuMask:
    case CompWindowTypeNormalMask:
      if (win_->mwmDecor() & (MwmDecorAll | MwmDecorTitle))
        return true;
  }

  return false;
}

bool Window::Impl::ShouldBeDecorated() const
{
  return (win_->frame() || win_->hasUnmapReference()) && FullyDecorated();
}

GLTexture* Window::Impl::ShadowTexture() const
{
  auto const& mi = manager_->impl_;
  return active() ? mi->active_shadow_pixmap_->texture() : mi->inactive_shadow_pixmap_->texture();
}

unsigned Window::Impl::ShadowRadius() const
{
  return active() ? manager_->active_shadow_radius() : manager_->inactive_shadow_radius();
}

void Window::Impl::RenderDecorationTexture(Side s, nux::Geometry const& geo)
{
  auto& deco_tex = bg_textures_[unsigned(s)];

  if (deco_tex.quad.box.width() != geo.width || deco_tex.quad.box.height() != geo.height)
  {
    cu::CairoContext ctx(geo.width, geo.height);
    auto ws = active() ? WidgetState::NORMAL : WidgetState::BACKDROP;
    Style::Get()->DrawSide(s, ws, ctx, geo.width, geo.height);
    deco_tex.SetTexture(ctx);
  }

  deco_tex.SetCoords(geo.x, geo.y);
}

void Window::Impl::UpdateDecorationTextures()
{
  if (!FullyDecorated())
  {
    bg_textures_.clear();
    return;
  }

  auto const& geo = win_->borderRect();
  auto const& border = win_->border();

  bg_textures_.resize(4);
  RenderDecorationTexture(Side::TOP, {geo.x(), geo.y(), geo.width(), border.top});
  RenderDecorationTexture(Side::LEFT, {geo.x(), geo.y() + border.top, border.left, geo.height() - border.top - border.bottom});
  RenderDecorationTexture(Side::RIGHT, {geo.x2() - border.right, geo.y() + border.top, border.right, geo.height() - border.top - border.bottom});
  RenderDecorationTexture(Side::BOTTOM, {geo.x(), geo.y2() - border.bottom, geo.width(), border.bottom});

  top_layout_->SetCoords(geo.x(), geo.y());
  top_layout_->SetSize(geo.width(), border.top);

  auto const& input = win_->inputRect();
  top_area_->SetCoords(input.x(), input.y());
  top_area_->SetSize(input.width(), input.height() / 2);
}

void Window::Impl::ComputeShadowQuads()
{
  if (!ShadowDecorated())
    return;

  auto* texture = ShadowTexture();

  if (!texture || !texture->width() || !texture->height())
    return;

  Quads& quads = shadow_quads_;
  auto const& tex_matrix = texture->matrix();
  auto const& border = win_->borderRect();
  auto const& offset = manager_->shadow_offset();
  int texture_offset = ShadowRadius() * 2;

  /* Top left quad */
  auto* quad = &quads[Quads::Pos::TOP_LEFT];
  quad->box.setGeometry(border.x() + offset.x - texture_offset,
                        border.y() + offset.y - texture_offset,
                        border.width() + offset.x + texture_offset * 2 - texture->width(),
                        border.height() + offset.y + texture_offset * 2 - texture->height());

  quad->matrix = tex_matrix;
  quad->matrix.x0 = 0.0f - COMP_TEX_COORD_X(quad->matrix, quad->box.x1());
  quad->matrix.y0 = 0.0f - COMP_TEX_COORD_Y(quad->matrix, quad->box.y1());

  /* Top right quad */
  quad = &quads[Quads::Pos::TOP_RIGHT];
  quad->box.setGeometry(quads[Quads::Pos::TOP_LEFT].box.x2(),
                        quads[Quads::Pos::TOP_LEFT].box.y1(),
                        texture->width(),
                        quads[Quads::Pos::TOP_LEFT].box.height());

  quad->matrix = tex_matrix;
  quad->matrix.xx = -1.0f / texture->width();
  quad->matrix.x0 = 1.0f - COMP_TEX_COORD_X(quad->matrix, quad->box.x1());
  quad->matrix.y0 = 0.0f - COMP_TEX_COORD_Y(quad->matrix, quad->box.y1());

  /* Bottom left */
  quad = &quads[Quads::Pos::BOTTOM_LEFT];
  quad->box.setGeometry(quads[Quads::Pos::TOP_LEFT].box.x1(),
                        quads[Quads::Pos::TOP_LEFT].box.y2(),
                        quads[Quads::Pos::TOP_LEFT].box.width(),
                        texture->height());

  quad->matrix = tex_matrix;
  quad->matrix.yy = -1.0f / texture->height();
  quad->matrix.x0 = 0.0f - COMP_TEX_COORD_X(quad->matrix, quad->box.x1());
  quad->matrix.y0 = 1.0f - COMP_TEX_COORD_Y(quad->matrix, quad->box.y1());

  /* Bottom right */
  quad = &quads[Quads::Pos::BOTTOM_RIGHT];
  quad->box.setGeometry(quads[Quads::Pos::BOTTOM_LEFT].box.x2(),
                        quads[Quads::Pos::TOP_RIGHT].box.y2(),
                        texture->width(),
                        texture->height());

  quad->matrix = tex_matrix;
  quad->matrix.xx = -1.0f / texture->width();
  quad->matrix.yy = -1.0f / texture->height();
  quad->matrix.x0 = 1.0f - COMP_TEX_COORD_X(quad->matrix, quad->box.x1());
  quad->matrix.y0 = 1.0f - COMP_TEX_COORD_Y(quad->matrix, quad->box.y1());

  /* Fix the quads if the texture is actually bigger than the area */
  if (texture->width() > border.width())
  {
    int half = win_->x() + win_->width() / 2.0f;
    quads[Quads::Pos::TOP_LEFT].box.setRight(half);
    quads[Quads::Pos::TOP_RIGHT].box.setLeft(half);
    quads[Quads::Pos::BOTTOM_LEFT].box.setRight(half);
    quads[Quads::Pos::BOTTOM_RIGHT].box.setLeft(half);
  }

  if (texture->height() > border.height())
  {
    int half = win_->y() + win_->height() / 2.0f;
    quads[Quads::Pos::TOP_LEFT].box.setBottom(half);
    quads[Quads::Pos::TOP_RIGHT].box.setBottom(half);
    quads[Quads::Pos::BOTTOM_LEFT].box.setTop(half);
    quads[Quads::Pos::BOTTOM_RIGHT].box.setTop(half);
  }

  CompRect shadows_rect;
  shadows_rect.setLeft(quads[Quads::Pos::TOP_LEFT].box.x1());
  shadows_rect.setTop(quads[Quads::Pos::TOP_LEFT].box.y1());
  shadows_rect.setRight(quads[Quads::Pos::TOP_RIGHT].box.x2());
  shadows_rect.setBottom(quads[Quads::Pos::BOTTOM_LEFT].box.y2());

  if (shadows_rect != last_shadow_rect_)
  {
    last_shadow_rect_ = shadows_rect;
    win_->updateWindowOutputExtents();
  }
}

void Window::Impl::Draw(GLMatrix const& transformation,
                        GLWindowPaintAttrib const& attrib,
                        CompRegion const& region, unsigned mask)
{
  if (!ShadowDecorated())
    return;

  auto const& clip_region = (mask & PAINT_WINDOW_TRANSFORMED_MASK) ? infiniteRegion : region;
  mask |= PAINT_WINDOW_BLEND_MASK;

  if (dirty_geo_)
    parent_->UpdateDecorationPosition();

  glwin_->vertexBuffer()->begin();

  for (unsigned i = 0; i < shadow_quads_.size(); ++i)
  {
    auto& quad = shadow_quads_[Quads::Pos(i)];
    glwin_->glAddGeometry({quad.matrix}, CompRegion(quad.box) - win_->region(), clip_region);
  }

  if (glwin_->vertexBuffer()->end())
    glwin_->glDrawTexture(ShadowTexture(), transformation, attrib, mask);

  for (auto const& dtex : bg_textures_)
  {
    glwin_->vertexBuffer()->begin();
    glwin_->glAddGeometry({dtex.quad.matrix}, dtex.quad.box, clip_region);

    if (glwin_->vertexBuffer()->end())
      glwin_->glDrawTexture(dtex, transformation, attrib, mask);
  }

  if (top_layout_)
    top_layout_->Draw(glwin_, transformation, attrib, region, mask);
}

// Public APIs

Window::Window(CompWindow* cwin)
  : impl_(new Impl(this, cwin))
{}

Window::~Window()
{}

void Window::Update()
{
  impl_->Update();
}

void Window::UpdateFrameRegion(CompRegion& r)
{
  if (impl_->frame_region_.isEmpty())
    return;

  auto const& geo = impl_->win_->geometry();
  auto const& input = impl_->win_->input();

  r += impl_->frame_region_.translated(geo.x() - input.left, geo.y() - input.top);
  UpdateDecorationPositionDelayed();
}

void Window::UpdateOutputExtents(compiz::window::extents::Extents& output)
{
  auto* win = impl_->win_;
  auto const& shadow = impl_->last_shadow_rect_;
  output.top = std::max(output.top, win->y() - shadow.y1());
  output.left = std::max(output.left, win->x() - shadow.x1());
  output.right = std::max(output.right, shadow.x2() - win->width() - win->x());
  output.bottom = std::max(output.bottom, shadow.y2() - win->height() - win->y());
}

void Window::Draw(GLMatrix const& matrix, GLWindowPaintAttrib const& attrib,
                  CompRegion const& region, unsigned mask)
{
  impl_->Draw(matrix, attrib, region, mask);
}

void Window::Undecorate()
{
  impl_->Undecorate();
}

void Window::UpdateDecorationPosition()
{
  impl_->ComputeShadowQuads();
  impl_->UpdateDecorationTextures();
  impl_->dirty_geo_ = false;
}

void Window::UpdateDecorationPositionDelayed()
{
  impl_->dirty_geo_ = true;
}

} // decoration namespace
} // unity namespace
