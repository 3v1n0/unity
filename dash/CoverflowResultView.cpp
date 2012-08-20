// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#include "CoverflowResultView.h"
#include "unity-shared/IconLoader.h"
#include "unity-shared/IconTexture.h"
#include <Nux/Nux.h>
#include <Nux/View.h>
#include <Nux/Coverflow.h>
#include <Nux/CoverflowModel.h>
#include <Nux/CoverflowItem.h>
#include <Nux/HLayout.h>


namespace unity
{
namespace dash
{

NUX_IMPLEMENT_OBJECT_TYPE(CoverflowResultView);

class CoverflowResultItem : public nux::CoverflowItem
{
public:
  CoverflowResultItem(Result& result, CoverflowResultView *parent);
  ~CoverflowResultItem();

  nux::ObjectPtr<nux::BaseTexture> GetTexture() const;
  void Activate();

  Result result_;
  CoverflowResultView* parent_;
  IconTexture *icon_texture_;
};

class CoverflowResultView::Impl : public sigc::trackable
{
public:
  Impl(CoverflowResultView *parent);
  ~Impl();

  void ComputeFlatIcons();

  CoverflowResultView *parent_;
  nux::Coverflow *coverflow_;
  nux::HLayout* layout_;
};

CoverflowResultItem::CoverflowResultItem(Result& result, CoverflowResultView *parent)
  : CoverflowItem(result.name())
  , result_(result)
  , parent_(parent)
{
  std::string const& icon_hint = result.icon_hint;
  std::string icon_name = !icon_hint.empty() ? icon_hint : ". GThemedIcon text-x-preview";
  static const int element_size = 128;
  
  icon_texture_ = new IconTexture(icon_name.c_str(), element_size, true);
  icon_texture_->LoadIcon();
  
  icon_texture_->texture_updated.connect([&] (nux::BaseTexture *texture)
  {
    if (parent_)
      parent_->NeedRedraw();
  });
}

CoverflowResultItem::~CoverflowResultItem()
{

}

nux::ObjectPtr<nux::BaseTexture> CoverflowResultItem::GetTexture() const
{
  return nux::ObjectPtr<nux::BaseTexture>(icon_texture_->texture());
}

void CoverflowResultItem::Activate()
{
  parent_->UriActivated.emit(result_.uri, ResultView::ActivateType::DIRECT);
}

CoverflowResultView::Impl::Impl(CoverflowResultView *parent)
  : parent_(parent)
  , coverflow_(new nux::Coverflow)
  , layout_(new nux::HLayout())
{
  layout_->AddView(coverflow_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  parent_->SetLayout(layout_);

  coverflow_->SetCameraDistance(1.44);
  coverflow_->fov = 60;
  coverflow_->folding_angle = 45;
  coverflow_->true_perspective = false;
  coverflow_->camera_motion_drift_enabled = false;
  coverflow_->show_reflection = true;
  coverflow_->folding_depth = .25;
  coverflow_->reflection_strength = .2f;
  //coverflow_->folding_rate = 3.5;
  coverflow_->kinetic_scroll_rate = 0.05f;
  coverflow_->mouse_drag_rate = 1.5f;
  coverflow_->pinching = 0.2f;
  coverflow_->y_offset = 0.15f;
  coverflow_->reflection_size = .5f;
}

CoverflowResultView::Impl::~Impl()
{
  
}

CoverflowResultView::CoverflowResultView(NUX_FILE_LINE_DECL)
  : ResultView(NUX_FILE_LINE_PARAM)
  , pimpl(new CoverflowResultView::Impl(this))
{
  SetMinimumHeight(180);
}

CoverflowResultView::~CoverflowResultView()
{
  
}

void CoverflowResultView::SetModelRenderer(ResultRenderer* renderer)
{
  return; // abstracting breaks down here. Needs to be reworked.  
}

void CoverflowResultView::AddResult(Result& result)
{
  nux::CoverflowItem::Ptr result_item(new CoverflowResultItem(result, this));
  pimpl->coverflow_->model()->AddItem(result_item);
}

void CoverflowResultView::RemoveResult(Result& result)
{
  ResultView::RemoveResult(result);
  // Ineffecient, API needs to be improved in Coverflow?
  for (nux::CoverflowItem::Ptr item : pimpl->coverflow_->model()->Items())
  {
    CoverflowResultItem *result_item = static_cast<CoverflowResultItem*>(item.GetPointer());

    if (result_item->result_.uri == result.uri) // maybe?
    {
      pimpl->coverflow_->model()->RemoveItem(item);
      break;
    }
  }  
}

void CoverflowResultView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  // do nothing here
}

void CoverflowResultView::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry base = GetGeometry();
  GfxContext.PushClippingRectangle(base);

  if (GetCompositionLayout())
  {
    nux::Geometry geo = GetCompositionLayout()->GetGeometry();
    GetCompositionLayout()->ProcessDraw(GfxContext, force_draw);
  }

  GfxContext.PopClippingRectangle();
}

void CoverflowResultView::Impl::ComputeFlatIcons()
{
  float width = coverflow_->ViewportWidthAtDepth(0);
  width /= coverflow_->space_between_icons();

  int flat_icons = static_cast<int>(std::floor((width - 2.0f) / 2.0f));
  coverflow_->flat_icons = flat_icons;
}

long CoverflowResultView::ComputeContentSize()
{
  pimpl->ComputeFlatIcons();
  long ret = ResultView::ComputeContentSize();
  return ret;
}


}
}
