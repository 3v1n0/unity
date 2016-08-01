// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013-2014 Canonical Ltd
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

#include <core/atoms.h>
#include <X11/Xatom.h>

#include "DecorationsPriv.h"
#include "DecorationsForceQuitDialog.h"
#include "DecorationsEdgeBorders.h"
#include "DecorationsGrabEdge.h"
#include "DecorationsWindowButton.h"
#include "DecorationsTitle.h"
#include "DecorationsSlidingLayout.h"
#include "DecorationsMenuLayout.h"
#include "WindowManager.h"
#include "UnitySettings.h"

namespace unity
{
namespace decoration
{
namespace
{
const std::string MENUS_PANEL_NAME = "WindowLIM";
const int SHADOW_BLUR_MARGIN_FACTOR = 2;
}

Window::Impl::Impl(Window* parent, CompWindow* win)
  : active(false)
  , parent_(parent)
  , win_(win)
  , cwin_(CompositeWindow::get(win_))
  , glwin_(GLWindow::get(win_))
  , frame_(0)
  , monitor_(0)
  , dirty_geo_(true)
  , dirty_frame_(false)
  , client_decorated_(false)
  , deco_elements_(cu::DecorationElement::NONE)
  , last_mwm_decor_(win_->mwmDecor())
  , last_actions_(win_->actions())
  , panel_id_(MENUS_PANEL_NAME + std::to_string(win_->id()))
  , cv_(Settings::Instance().em())
{
  active.changed.connect([this] (bool active) {
    bg_textures_.clear();
    if (top_layout_)
      top_layout_->focused = active;
    RedrawDecorations();
  });

  parent->title.SetGetterFunction([this] {
    if (title_)
      return title_->text();

    if (last_title_.empty())
      last_title_ = WindowManager::Default().GetWindowName(win_->id());

    return last_title_;
  });

  parent->title.SetSetterFunction([this] (std::string const& new_title) {
    auto const& title_ptr = title_.lock();

    if (!title_ptr)
    {
      if (last_title_ != new_title)
      {
        last_title_ = new_title;
        return true;
      }

      return false;
    }

    if (new_title == title_ptr->text())
      return false;

    title_ptr->text = new_title;
    return true;
  });

  parent->dpi_scale.SetGetterFunction([this] { return cv_->DPIScale(); });
  parent->scaled.changed.connect(sigc::hide(sigc::mem_fun(this, &Impl::Update)));

  if (win_->isViewable() || win_->shaded())
    Update();
}

Window::Impl::~Impl()
{
  Undecorate();
}

void Window::Impl::Update()
{
  UpdateClientDecorationsState();
  UpdateElements(client_decorated_ ? cu::WindowFilter::CLIENTSIDE_DECORATED : cu::WindowFilter::NONE);

  if (deco_elements_ & (cu::DecorationElement::EDGE | cu::DecorationElement::BORDER))
    Decorate();
  else
    Undecorate();

  last_mwm_decor_ = win_->mwmDecor();
  last_actions_ = win_->actions();
}

void Window::Impl::Decorate()
{
  SetupExtents();
  UpdateFrame();
  SetupWindowEdges();

  if (deco_elements_ & cu::DecorationElement::BORDER)
  {
    SetupWindowControls();
  }
  else
  {
    CleanupWindowControls();
    bg_textures_.clear();
  }
}

void Window::Impl::Undecorate()
{
  UnsetExtents();
  UnsetFrame();
  CleanupWindowControls();
  CleanupWindowEdges();
  bg_textures_.clear();
}

void Window::Impl::UnsetExtents()
{
  if (win_->hasUnmapReference())
    return;

  CompWindowExtents empty;

  if (win_->border() != empty || win_->input() != empty)
    win_->setWindowFrameExtents(&empty, &empty);
}

void Window::Impl::ComputeBorderExtent(CompWindowExtents& border)
{
  if (deco_elements_ & cu::DecorationElement::BORDER)
  {
    auto const& sb = Style::Get()->Border();
    border.left = cv_->CP(sb.left);
    border.right = cv_->CP(sb.right);
    border.top = cv_->CP(sb.top);
    border.bottom = cv_->CP(sb.bottom);
  }
}

void Window::Impl::SetupExtents()
{
  if (win_->hasUnmapReference())
    return;

  CompWindowExtents border;
  ComputeBorderExtent(border);

  CompWindowExtents input(border);

  if (deco_elements_ & cu::DecorationElement::EDGE)
  {
    auto const& ib = Style::Get()->InputBorder();
    input.left += cv_->CP(ib.left);
    input.right += cv_->CP(ib.right);
    input.top += cv_->CP(ib.top);
    input.bottom += cv_->CP(ib.bottom);
  }

  if (win_->border() != border || win_->input() != input)
    win_->setWindowFrameExtents(&border, &input);
}

void Window::Impl::SendFrameExtents()
{
  UpdateElements(cu::WindowFilter::UNMAPPED);

  CompWindowExtents border;
  ComputeBorderExtent(border);

  std::vector<unsigned long> extents(4);
  extents.push_back(border.left);
  extents.push_back(border.right);
  extents.push_back(border.top);
  extents.push_back(border.bottom);

  XChangeProperty(screen->dpy(), win_->id(), Atoms::frameExtents, XA_CARDINAL, 32,
                  PropModeReplace, reinterpret_cast<unsigned char *>(extents.data()),
                  extents.size());
}

void Window::Impl::UnsetFrame()
{
  if (!frame_)
    return;

  XDestroyWindow(screen->dpy(), frame_);
  framed.emit(false, frame_);
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

  if (!frame_ && win_->frame())
    CreateFrame(frame_geo);

  if (frame_ && frame_geo_ != frame_geo)
    UpdateFrameGeo(frame_geo);
}

void Window::Impl::UpdateFrameActions()
{
  if (!dirty_frame_ && (win_->mwmDecor() != last_mwm_decor_ || win_->actions() != last_actions_))
  {
    dirty_frame_ = true;
    Damage();
  }
}

void Window::Impl::CreateFrame(nux::Geometry const& frame_geo)
{
  /* Since we're reparenting windows here, we need to grab the server
   * which sucks, but its necessary */
  Display* dpy = screen->dpy();
  XGrabServer(dpy);

  XSetWindowAttributes attr;
  attr.event_mask = StructureNotifyMask | ButtonPressMask | ButtonReleaseMask |
                    EnterWindowMask | LeaveWindowMask | PointerMotionMask;
  attr.override_redirect = True;

  auto parent = win_->frame();
  frame_ = XCreateWindow(dpy, parent, frame_geo.x, frame_geo.y,
                         frame_geo.width, frame_geo.height, 0, CopyFromParent,
                         InputOnly, CopyFromParent, CWOverrideRedirect | CWEventMask, &attr);

  if (screen->XShape())
    XShapeSelectInput(dpy, frame_, ShapeNotifyMask);

  XMapWindow(dpy, frame_);
  framed.emit(true, frame_);

  XUngrabServer(dpy);
}

void Window::Impl::UpdateFrameGeo(nux::Geometry const& frame_geo)
{
  auto const& input = win_->input();
  Display* dpy = screen->dpy();
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

void Window::Impl::SyncXShapeWithFrameRegion()
{
  frame_region_ = CompRegion();

  int n = 0;
  int order = 0;
  XRectangle *rects = nullptr;

  rects = XShapeGetRectangles(screen->dpy(), frame_, ShapeInput, &n, &order);
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

void Window::Impl::SetupWindowEdges()
{
  if (input_mixer_)
    return;

  dpi_changed_ = Settings::Instance().dpi_changed.connect([this] {
    Update();
    edge_borders_->scale = cv_->DPIScale();
    if (top_layout_) top_layout_->scale = cv_->DPIScale();
  });

  input_mixer_ = std::make_shared<InputMixer>();
  edge_borders_ = std::make_shared<EdgeBorders>(win_);
  edge_borders_->scale = cv_->DPIScale();
  input_mixer_->PushToFront(edge_borders_);

  UpdateWindowEdgesGeo();
}

void Window::Impl::UpdateWindowEdgesGeo()
{
  if (!edge_borders_)
    return;

  auto const& input = win_->inputRect();
  edge_borders_->SetCoords(input.x(), input.y());
  edge_borders_->SetSize(input.width(), input.height());
}

void Window::Impl::CleanupWindowEdges()
{
  input_mixer_.reset();
  edge_borders_.reset();
  dpi_changed_->disconnect();
}

void Window::Impl::SetupWindowControls()
{
  if (top_layout_)
    return;

  auto const& style = Style::Get();
  theme_changed_ = style->theme.changed.connect([this] (std::string const&) {
    Undecorate();
    Decorate();
  });

  grab_edge_ = std::static_pointer_cast<EdgeBorders>(edge_borders_)->GetEdge(Edge::Type::GRAB);

  auto padding = style->Padding(Side::TOP);
  top_layout_ = std::make_shared<Layout>();
  top_layout_->left_padding = padding.left;
  top_layout_->right_padding = padding.right;
  top_layout_->top_padding = padding.top;
  top_layout_->focused = active();
  top_layout_->scale = cv_->DPIScale();

  if (win_->actions() & CompWindowActionCloseMask)
    top_layout_->Append(std::make_shared<WindowButton>(win_, WindowButtonType::CLOSE));

  if (win_->actions() & CompWindowActionMinimizeMask)
    top_layout_->Append(std::make_shared<WindowButton>(win_, WindowButtonType::MINIMIZE));

  if (win_->actions() & (CompWindowActionMaximizeHorzMask|CompWindowActionMaximizeVertMask))
    top_layout_->Append(std::make_shared<WindowButton>(win_, WindowButtonType::MAXIMIZE));

  auto title = std::make_shared<Title>();
  title->text = last_title_.empty() ? WindowManager::Default().GetWindowName(win_->id()) : last_title_;
  title->sensitive = false;
  title_ = title;
  last_title_.clear();

  auto sliding_layout = std::make_shared<SlidingLayout>();
  sliding_layout->SetMainItem(title);
  sliding_layout_ = sliding_layout;

  auto title_layout = std::make_shared<Layout>();
  title_layout->left_padding = style->TitleIndent();
  title_layout->Append(sliding_layout);
  top_layout_->Append(title_layout);

  input_mixer_->PushToFront(top_layout_);
  dirty_frame_ = false;

  SetupAppMenu();
  RedrawDecorations();
}

void Window::Impl::CleanupWindowControls()
{
  if (title_)
    last_title_ = title_->text();

  if (input_mixer_)
    input_mixer_->Remove(top_layout_);

  UnsetAppMenu();
  theme_changed_->disconnect();
  top_layout_.reset();
}

bool Window::Impl::IsMaximized() const
{
  return (win_->state() & MAXIMIZE_STATE) == MAXIMIZE_STATE;
}

void Window::Impl::UpdateElements(cu::WindowFilter wf)
{
  if (!parent_->scaled() && IsMaximized())
  {
    deco_elements_ = cu::DecorationElement::NONE;
    return;
  }

  deco_elements_ = cu::WindowDecorationElements(win_, wf);
}

void Window::Impl::UpdateClientDecorationsState()
{
  if (win_->alpha())
  {
    auto const& corners = WindowManager::Default().GetCardinalProperty(win_->id(), atom::_UNITY_GTK_BORDER_RADIUS);

    if (!corners.empty())
    {
      enum Corner { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT };
      client_borders_.top = std::max(corners[TOP_LEFT], corners[TOP_RIGHT]);
      client_borders_.left = std::max(corners[TOP_LEFT], corners[BOTTOM_LEFT]);
      client_borders_.right = std::max(corners[TOP_RIGHT], corners[BOTTOM_RIGHT]);
      client_borders_.bottom = std::max(corners[BOTTOM_LEFT], corners[BOTTOM_RIGHT]);
      client_decorated_ = true;
      return;
    }
  }

  if (client_decorated_)
  {
    client_borders_ = CompWindowExtents();
    client_decorated_ = false;
  }
}

bool Window::Impl::ShadowDecorated() const
{
  return deco_elements_ & cu::DecorationElement::SHADOW;
}

bool Window::Impl::ShapedShadowDecorated() const
{
  return deco_elements_ & cu::DecorationElement::SHADOW &&
         deco_elements_ & cu::DecorationElement::SHAPED;
}

bool Window::Impl::FullyDecorated() const
{
  return deco_elements_ & cu::DecorationElement::BORDER;
}

bool Window::Impl::ShouldBeDecorated() const
{
  return (win_->frame() || win_->hasUnmapReference()) && FullyDecorated();
}

GLTexture* Window::Impl::ShadowTexture() const
{
  if (shaped_shadow_pixmap_)
    return shaped_shadow_pixmap_->texture();

  return SharedShadowTexture();
}

GLTexture* Window::Impl::SharedShadowTexture() const
{
  auto const& mi = manager_->impl_;
  if (active() || parent_->scaled())
    return mi->active_shadow_pixmap_->texture();

  return mi->inactive_shadow_pixmap_->texture();
}

unsigned Window::Impl::ShadowRadius() const
{
  if (active() || parent_->scaled())
    return manager_->active_shadow_radius();

  return manager_->inactive_shadow_radius();
}

void Window::Impl::RenderDecorationTexture(Side s, nux::Geometry const& geo)
{
  if (geo.width <= 0 || geo.height <= 0)
    return;

  auto& deco_tex = bg_textures_[unsigned(s)];

  if (deco_tex.quad.box.width() != geo.width || deco_tex.quad.box.height() != geo.height)
  {
    double scale = top_layout_->scale();
    cu::CairoContext ctx(geo.width, geo.height, scale);
    auto ws = active() ? WidgetState::NORMAL : WidgetState::BACKDROP;
    Style::Get()->DrawSide(s, ws, ctx, geo.width / scale, geo.height / scale);
    deco_tex.SetTexture(ctx);
  }

  deco_tex.SetCoords(geo.x, geo.y);
  deco_tex.quad.region = deco_tex.quad.box;
}

void Window::Impl::UpdateDecorationTextures()
{
  if (!top_layout_)
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

  SyncMenusGeometries();
}

void Window::Impl::ComputeShadowQuads()
{
  if (!(deco_elements_ & cu::DecorationElement::SHADOW))
  {
    if (!last_shadow_rect_.isEmpty())
      last_shadow_rect_.setGeometry(0, 0, 0, 0);

    shaped_shadow_pixmap_.reset();
  }
  else if (deco_elements_ & cu::DecorationElement::SHAPED)
  {
    ComputeShapedShadowQuad();
  }
  else
  {
    shaped_shadow_pixmap_.reset();
    ComputeGenericShadowQuads();
  }
}

void Window::Impl::ComputeGenericShadowQuads()
{
  const auto* texture = SharedShadowTexture();

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
    auto win_region = win_->region();

    if (client_decorated_)
    {
      win_region.shrink(client_borders_.left + client_borders_.right, client_borders_.top + client_borders_.bottom);
      win_region.translate(client_borders_.left - client_borders_.right, client_borders_.top - client_borders_.bottom);
    }

    quads[Quads::Pos::TOP_LEFT].region = CompRegion(quads[Quads::Pos::TOP_LEFT].box) - win_region;
    quads[Quads::Pos::TOP_RIGHT].region = CompRegion(quads[Quads::Pos::TOP_RIGHT].box) - win_region;
    quads[Quads::Pos::BOTTOM_LEFT].region = CompRegion(quads[Quads::Pos::BOTTOM_LEFT].box) - win_region;
    quads[Quads::Pos::BOTTOM_RIGHT].region = CompRegion(quads[Quads::Pos::BOTTOM_RIGHT].box) - win_region;

    last_shadow_rect_ = shadows_rect;
    win_->updateWindowOutputExtents();
  }
}

cu::PixmapTexture::Ptr Window::Impl::BuildShapedShadowTexture(nux::Size const& size, unsigned radius, nux::Color const& color, Shape const& shape) {
  nux::CairoGraphics img(CAIRO_FORMAT_ARGB32, size.width, size.height);
  auto* img_ctx = img.GetInternalContext();

  for (auto const& rect : shape.GetRectangles())
  {
    cairo_rectangle(img_ctx, rect.x + radius * SHADOW_BLUR_MARGIN_FACTOR - shape.XOffset(), rect.y + radius * SHADOW_BLUR_MARGIN_FACTOR - shape.YOffset(), rect.width, rect.height);
    cairo_set_source_rgba(img_ctx, color.red, color.green, color.blue, color.alpha);
    cairo_fill(img_ctx);
  }

  img.BlurSurface(radius);

  cu::CairoContext shadow_ctx(size.width, size.height);
  cairo_set_source_surface(shadow_ctx, img.GetSurface(), 0, 0);
  cairo_paint(shadow_ctx);

  return shadow_ctx;
}

void Window::Impl::ComputeShapedShadowQuad()
{
  nux::Color color = active() ? manager_->active_shadow_color() : manager_->inactive_shadow_color();
  unsigned int radius = active() ? manager_->active_shadow_radius() : manager_->inactive_shadow_radius();

  Shape shape(win_->id());
  auto const& border = win_->borderRect();
  auto const& shadow_offset = manager_->shadow_offset();

  // Ideally it would be shape.getWidth + radius * 2 but Cairographics::BlurSurface
  // isn't bounded by the radius and we need to compensate by using a larger texture.
  int width = shape.Width() + radius * 2 * SHADOW_BLUR_MARGIN_FACTOR;
  int height = shape.Height() + radius * 2 * SHADOW_BLUR_MARGIN_FACTOR;

  if (width != last_shadow_rect_.width() || height != last_shadow_rect_.height())
    shaped_shadow_pixmap_ = BuildShapedShadowTexture({width, height}, radius, color, shape);

  const auto* texture = shaped_shadow_pixmap_->texture();

  if (!texture || !texture->width() || !texture->height())
    return;

  int x = border.x() + shadow_offset.x - radius * 2 + shape.XOffset();
  int y = border.y() + shadow_offset.y - radius * 2 + shape.YOffset();

  auto* quad = &shadow_quads_[Quads::Pos(0)];
  quad->box.setGeometry(x, y, width, height);
  quad->matrix = texture->matrix();
  quad->matrix.x0 = -COMP_TEX_COORD_X(quad->matrix, quad->box.x1());
  quad->matrix.y0 = -COMP_TEX_COORD_Y(quad->matrix, quad->box.y1());

  CompRect shaped_shadow_rect(x, y, width, height);
  if (shaped_shadow_rect != last_shadow_rect_)
  {
    auto const& win_region = win_->region();
    quad->region = CompRegion(quad->box) - win_region;

    last_shadow_rect_ = shaped_shadow_rect;
    win_->updateWindowOutputExtents();
  }
}

void Window::Impl::Paint(GLMatrix const& transformation,
                         GLWindowPaintAttrib const& attrib,
                         CompRegion const& region, unsigned mask)
{
  if (!(mask & PAINT_SCREEN_TRANSFORMED_MASK) && win_->defaultViewport() != screen->vp())
  {
    return;
  }

  if (dirty_geo_)
    parent_->UpdateDecorationPosition();

  if (dirty_frame_)
  {
    dirty_frame_ = false;
    CleanupWindowControls();
    CleanupWindowEdges();
    Update();
  }
}

void Window::Impl::Draw(GLMatrix const& transformation,
                        GLWindowPaintAttrib const& attrib,
                        CompRegion const& region, unsigned mask)
{
  if (last_shadow_rect_.isEmpty() || (!(mask & PAINT_SCREEN_TRANSFORMED_MASK) && win_->defaultViewport() != screen->vp()))
  {
    return;
  }

  auto const& clip_region = (mask & PAINT_WINDOW_TRANSFORMED_MASK) ? infiniteRegion : region;
  mask |= PAINT_WINDOW_BLEND_MASK;

  if (win_->alpha() || attrib.opacity != OPAQUE)
    mask |= PAINT_WINDOW_TRANSLUCENT_MASK;

  glwin_->vertexBuffer()->begin();

  unsigned int num_quads = ShapedShadowDecorated() ? 1 : shadow_quads_.size();
  for (unsigned int i = 0; i < num_quads; ++i)
  {
    auto& quad = shadow_quads_[Quads::Pos(i)];
    glwin_->glAddGeometry(quad.matrices, quad.region, clip_region);
  }

  if (glwin_->vertexBuffer()->end())
  {
    if (GLTexture* texture = ShadowTexture())
      glwin_->glDrawTexture(texture, transformation, attrib, mask);
  }

  for (auto const& dtex : bg_textures_)
  {
    if (!dtex)
      continue;

    glwin_->vertexBuffer()->begin();
    glwin_->glAddGeometry(dtex.quad.matrices, dtex.quad.region, clip_region);

    if (glwin_->vertexBuffer()->end())
      glwin_->glDrawTexture(dtex, transformation, attrib, mask);
  }

  if (top_layout_)
    top_layout_->Draw(glwin_, transformation, attrib, region, mask);
}

void Window::Impl::Damage()
{
  cwin_->damageOutputExtents();
}

void Window::Impl::RedrawDecorations()
{
  if (!win_->isMapped())
    return;

  dirty_geo_ = true;
  cwin_->damageOutputExtents();
}

void Window::Impl::SetupAppMenu()
{
  if (!top_layout_)
    return;

  auto const& menu_manager = manager_->impl_->menu_manager_;
  auto const& sliding_layout = sliding_layout_.lock();
  sliding_layout->SetInputItem(nullptr);
  sliding_layout->mouse_owner = false;
  sliding_layout->override_main_item = false;
  grab_mouse_changed_->disconnect();

  if (!menu_manager->HasAppMenu() || !menu_manager->integrated_menus())
    return;

  auto menus = std::make_shared<MenuLayout>(menu_manager, win_);
  menus->Setup();

  if (menus->Items().empty())
    return;

  menus_ = menus;
  auto const& grab_edge = grab_edge_.lock();
  sliding_layout->SetInputItem(menus);
  sliding_layout->fadein = menu_manager->fadein();
  sliding_layout->fadeout = menu_manager->fadeout();

  if (menu_manager->always_show_menus())
  {
    sliding_layout->override_main_item = true;
  }
  else
  {
    auto visibility_cb = sigc::hide(sigc::mem_fun(this, &Impl::UpdateAppMenuVisibility));
    menus->active.changed.connect(visibility_cb);
    menus->show_now.changed.connect(visibility_cb);
    menus->mouse_owner.changed.connect(visibility_cb);

    if (grab_edge->mouse_owner() || grab_edge->Geometry().contains(CompPoint(pointerX, pointerY)))
      sliding_layout->mouse_owner = true;

    grab_mouse_changed_ = grab_edge->mouse_owner.changed.connect([this] (bool owner) {
      sliding_layout_->mouse_owner = owner || menus_->show_now();
    });

    if (sliding_layout->mouse_owner())
      input_mixer_->ForceMouseOwnerCheck();
  }

  SyncMenusGeometries();
}

void Window::Impl::UpdateAppMenuVisibility()
{
  auto const& sliding_layout = sliding_layout_.lock();
  auto const& menus = menus_.lock();

  sliding_layout->mouse_owner = (menus->mouse_owner() || menus->active() || menus->show_now());

  if (!sliding_layout->mouse_owner())
    sliding_layout->mouse_owner = grab_edge_->mouse_owner();
}

inline std::string const& Window::Impl::GetMenusPanelID() const
{
  return panel_id_;
}

void Window::Impl::UnsetAppMenu()
{
  if (!menus_)
    return;

  auto const& indicators = manager_->impl_->menu_manager_->Indicators();
  indicators->SyncGeometries(GetMenusPanelID(), indicator::EntryLocationMap());
  sliding_layout_->SetInputItem(nullptr);
  grab_mouse_changed_->disconnect();
}

void Window::Impl::SyncMenusGeometries() const
{
  if (!menus_)
    return;

  auto const& indicators = manager_->impl_->menu_manager_->Indicators();
  indicator::EntryLocationMap map;
  menus_->ChildrenGeometries(map);
  indicators->SyncGeometries(GetMenusPanelID(), map);
}

bool Window::Impl::ActivateMenu(std::string const& entry_id)
{
  return menus_ && menus_->ActivateMenu(entry_id);
}

void Window::Impl::UpdateMonitor()
{
  auto const& input = win_->inputRect();
  nux::Geometry window_geo(input.x(), input.y(), input.width(), input.height());

  int monitor = WindowManager::Default().MonitorGeometryIn(window_geo);

  if (monitor_ != monitor)
  {
    monitor_ = monitor;
    cv_ = unity::Settings::Instance().em(monitor);
    Update();

    if (top_layout_)
      top_layout_->scale = cv_->DPIScale();

    if (edge_borders_)
      edge_borders_->scale = cv_->DPIScale();
  }
}

void Window::Impl::UpdateForceQuitDialogPosition()
{
  if (force_quit_)
    force_quit_->UpdateDialogPosition();
}

void Window::Impl::ShowForceQuitDialog(bool show, Time time)
{
  if (show)
  {
    if (!force_quit_)
    {
      force_quit_ = std::make_shared<ForceQuitDialog>(win_, time);
      force_quit_->close_request.connect([this] { force_quit_.reset(); });
    }

    force_quit_->time = time;
  }
  else
  {
    force_quit_.reset();
  }
}

// Public APIs

Window::Window(CompWindow* cwin)
  : scaled(false)
  , impl_(new Impl(this, cwin))
{}

CompWindow* Window::GetCompWindow()
{
  return impl_->win_;
}

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
  const auto* win = impl_->win_;
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

void Window::Paint(GLMatrix const& matrix, GLWindowPaintAttrib const& attrib,
                  CompRegion const& region, unsigned mask)
{
  impl_->Paint(matrix, attrib, region, mask);
}

void Window::Undecorate()
{
  impl_->Undecorate();
}

void Window::UpdateDecorationPosition()
{
  impl_->UpdateMonitor();
  impl_->ComputeShadowQuads();
  impl_->UpdateWindowEdgesGeo();
  impl_->UpdateDecorationTextures();
  impl_->UpdateForceQuitDialogPosition();
  impl_->dirty_geo_ = false;
}

void Window::UpdateDecorationPositionDelayed()
{
  impl_->dirty_geo_ = true;
}

std::string Window::GetName() const
{
  return "DecoratedWindow";
}

void Window::AddProperties(debug::IntrospectionData& data)
{
  data.add(impl_->win_->borderRect())
  .add("input_geo", impl_->win_->inputRect())
  .add("content_geo", impl_->win_->geometry())
  .add("region", impl_->win_->region().boundingRect())
  .add("title", title())
  .add("active", impl_->active())
  .add("scaled", scaled())
  .add("monitor", impl_->monitor_)
  .add("dpi_scale", dpi_scale())
  .add("xid", impl_->win_->id())
  .add("fully_decorable", cu::IsWindowFullyDecorable(impl_->win_))
  .add("shadow_decorable", cu::IsWindowShadowDecorable(impl_->win_))
  .add("shadow_decorated", impl_->ShadowDecorated())
  .add("fully_decorated", impl_->FullyDecorated())
  .add("should_be_decorated", impl_->ShouldBeDecorated())
  .add("framed", (impl_->frame_ != 0))
  .add("frame_geo", impl_->frame_geo_)
  .add("shadow_rect", impl_->last_shadow_rect_)
  .add("maximized", impl_->IsMaximized())
  .add("v_maximized", (impl_->win_->state() & CompWindowStateMaximizedVertMask))
  .add("h_maximized", (impl_->win_->state() & CompWindowStateMaximizedHorzMask))
  .add("resizable", (impl_->win_->actions() & CompWindowActionResizeMask))
  .add("movable", (impl_->win_->actions() & CompWindowActionMoveMask))
  .add("closable", (impl_->win_->actions() & CompWindowActionCloseMask))
  .add("minimizable", (impl_->win_->actions() & CompWindowActionMinimizeMask))
  .add("maximizable", (impl_->win_->actions() & (CompWindowActionMaximizeHorzMask|CompWindowActionMaximizeVertMask)));
}

debug::Introspectable::IntrospectableList Window::GetIntrospectableChildren()
{
  return IntrospectableList({impl_->top_layout_.get(), impl_->edge_borders_.get()});
}

} // decoration namespace
} // unity namespace
