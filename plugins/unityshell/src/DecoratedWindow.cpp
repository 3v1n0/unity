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

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>

#include "DecorationsManagerPriv.h"

namespace unity
{
namespace decoration
{

namespace
{
DECLARE_LOGGER(logger, "unity.decoration.window");
const unsigned GRAB_BORDER = 10;
}

Window::Impl::Impl(Window* parent, UnityWindow* uwin)
  : active(false)
  , parent_(parent)
  , uwin_(uwin)
  , frame_(0)
  , dirty_geo_(true)
{
  auto* window = uwin_->window;

  if (window->isViewable() || window->shaded())
  {
    window->resizeNotifySetEnabled(uwin_, false);
    Update();
    window->resizeNotifySetEnabled(uwin_, true);
  }

  active.changed.connect([this] (bool) {
    bg_textures_.clear();
    parent_->UpdateDecorationPositionDelayed();
    uwin_->cWindow->damageOutputExtents();
  });
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
  bg_textures_.clear();
}

void Window::Impl::UnsetExtents()
{
  auto* window = uwin_->window;

  if (window->hasUnmapReference())
    return;

  CompWindowExtents empty(0, 0, 0, 0);

  if (window->border() != empty || window->input() != empty)
    window->setWindowFrameExtents(&empty, &empty);
}

void Window::Impl::SetupExtents()
{
  auto* window = uwin_->window;

  if (window->hasUnmapReference())
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

  if (window->border() != border || window->input() != input)
    window->setWindowFrameExtents(&border, &input);
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
  auto* window = uwin_->window;
  auto const& input = window->input();
  auto const& server = window->serverGeometry();
  nux::Geometry frame_geo(0, 0, server.widthIncBorders() + input.left + input.right,
                          server.heightIncBorders() + input.top  + input.bottom);

  if (window->shaded())
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

    auto parent = window->frame();
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

  // uwin_->cWindow->damageOutputExtents();
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

  uwin_->window->updateFrameRegion();
}

bool Window::Impl::ShadowDecorated() const
{
  auto* window = uwin_->window;

  if (!window->isViewable())
    return false;

  if ((window->state() & MAXIMIZE_STATE) == MAXIMIZE_STATE)
    return false;

  if (window->wmType() & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
    return false;

  if (window->region().numRects() != 1) // Non rectangular windows
    return false;

  if (window->overrideRedirect() && window->alpha())
    return false;

  return true;
}

bool Window::Impl::FullyDecorated() const
{
  if (!ShadowDecorated())
    return false;

  if (uwin_->window->overrideRedirect())
    return false;

  switch (uwin_->window->type())
  {
    case CompWindowTypeDialogMask:
    case CompWindowTypeModalDialogMask:
    case CompWindowTypeUtilMask:
    case CompWindowTypeMenuMask:
    case CompWindowTypeNormalMask:
      if (uwin_->window->mwmDecor() & (MwmDecorAll | MwmDecorTitle))
        return true;
  }

  return false;
}

bool Window::Impl::ShouldBeDecorated() const
{
  auto* window = uwin_->window;
  return (window->frame() || window->hasUnmapReference()) && FullyDecorated();
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

  auto *window = uwin_->window;
  auto const& geo = window->borderRect();
  auto const& border = window->border();

  bg_textures_.resize(4);
  RenderDecorationTexture(Side::TOP, {geo.x(), geo.y(), geo.width(), window->border().top});
  RenderDecorationTexture(Side::LEFT, {geo.x(), geo.y() + border.top, border.left, geo.height() - border.top - border.bottom});
  RenderDecorationTexture(Side::RIGHT, {geo.x2() - border.right, geo.y() + border.top, border.right, geo.height() - border.top - border.bottom});
  RenderDecorationTexture(Side::BOTTOM, {geo.x(), geo.y2() - border.bottom, geo.width(), border.bottom});
}

void Window::Impl::ComputeShadowQuads()
{
  if (!ShadowDecorated())
    return;

  auto* texture = ShadowTexture();

  if (!texture || !texture->width() || !texture->height())
    return;

  Quads& quads = shadow_quads_;
  auto *window = uwin_->window;
  auto const& tex_matrix = texture->matrix();
  auto const& border = window->borderRect();
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
    int half = window->x() + window->width() / 2.0f;
    quads[Quads::Pos::TOP_LEFT].box.setRight(half);
    quads[Quads::Pos::TOP_RIGHT].box.setLeft(half);
    quads[Quads::Pos::BOTTOM_LEFT].box.setRight(half);
    quads[Quads::Pos::BOTTOM_RIGHT].box.setLeft(half);
  }

  if (texture->height() > border.height())
  {
    int half = window->y() + window->height() / 2.0f;
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
    window->updateWindowOutputExtents();
  }
}

void Window::Impl::Draw(GLMatrix const& transformation,
                        GLWindowPaintAttrib const& attrib,
                        CompRegion const& region, unsigned mask)
{
  if (!ShadowDecorated())
    return;

  auto* window = uwin_->window;
  auto* gWindow = uwin_->gWindow;
  auto const& clip_region = (mask & PAINT_WINDOW_TRANSFORMED_MASK) ? infiniteRegion : region;
  mask |= PAINT_WINDOW_BLEND_MASK;

  if (dirty_geo_)
    parent_->UpdateDecorationPosition();

  gWindow->vertexBuffer()->begin();

  for (unsigned i = 0; i < shadow_quads_.size(); ++i)
  {
    auto& quad = shadow_quads_[Quads::Pos(i)];
    gWindow->glAddGeometry({quad.matrix}, CompRegion(quad.box) - window->region(), clip_region);
  }

  if (gWindow->vertexBuffer()->end())
    gWindow->glDrawTexture(ShadowTexture(), transformation, attrib, mask);

  for (auto const& dtex : bg_textures_)
  {
    gWindow->vertexBuffer()->begin();
    gWindow->glAddGeometry({dtex.quad.matrix}, dtex.quad.box, clip_region);

    if (gWindow->vertexBuffer()->end())
      gWindow->glDrawTexture(dtex, transformation, attrib, mask);
  }
}

Window::Window(UnityWindow* uwin)
  : impl_(new Impl(this, uwin))
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

  auto* window = impl_->uwin_->window;
  auto const& geo = window->geometry();
  auto const& input = window->input();

  r += impl_->frame_region_.translated(geo.x() - input.left, geo.y() - input.top);
  impl_->dirty_geo_ = true;
}

void Window::UpdateOutputExtents(compiz::window::extents::Extents& output)
{
  auto* window = impl_->uwin_->window;
  auto const& shadow = impl_->last_shadow_rect_;
  output.top = std::max(output.top, window->y() - shadow.y1());
  output.left = std::max(output.left, window->x() - shadow.x1());
  output.right = std::max(output.right, shadow.x2() - window->width() - window->x());
  output.bottom = std::max(output.bottom, shadow.y2() - window->height() - window->y());
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
