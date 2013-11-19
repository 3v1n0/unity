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
#include "unity-shared/DashStyle.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/GraphicsUtils.h"
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
  CoverflowResultItem(Result& result, CoverflowResultView *parent, nux::CoverflowModel::Ptr model);
  virtual ~CoverflowResultItem();

  nux::ObjectPtr<nux::BaseTexture> GetTexture() const;
  virtual void Activate(int button);

  int Index();
  std::string Uri();

  Result result_;
  CoverflowResultView* parent_;
  nux::CoverflowModel::Ptr model_;
  IconTexture *icon_texture_;
  UBusManager ubus_;
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
  UBusManager ubus_;
};

CoverflowResultItem::CoverflowResultItem(Result& result, CoverflowResultView *parent, nux::CoverflowModel::Ptr model)
  : CoverflowItem(result.name())
  , result_(result)
  , parent_(parent)
  , model_(model)
{
  Style& style = Style::Instance();
  std::string const& icon_hint = result.icon_hint;
  std::string icon_name = !icon_hint.empty() ? icon_hint : ". GThemedIcon text-x-preview";
  static const int element_size = style.GetTileHeight();

  icon_texture_ = new IconTexture(icon_name.c_str(), element_size, true);
  icon_texture_->SinkReference();
  icon_texture_->LoadIcon();

  icon_texture_->texture_updated.connect([this] (nux::ObjectPtr<nux::BaseTexture> const&)
  {
    if (parent_)
      parent_->NeedRedraw();
  });
}

CoverflowResultItem::~CoverflowResultItem()
{
  icon_texture_->UnReference();
}

std::string CoverflowResultItem::Uri()
{
  return result_.uri();
}

int CoverflowResultItem::Index()
{
  int i = 0;
  for (auto item : model_->Items())
  {
    if (this == item.GetPointer())
      return i;
    i++;
  }
  return -1;
}

nux::ObjectPtr<nux::BaseTexture> CoverflowResultItem::GetTexture() const
{
  return icon_texture_->texture();
}

void CoverflowResultItem::Activate(int button)
{
  int index = Index();
  int size = model_->Items().size();

  glib::Variant data(g_variant_new("(iiii)", 0, 0, index, size - index));

  //Left and right click take you to previews.
  if (button == 1 || button == 3)
    parent_->Activate(LocalResult(result_), index, ResultView::ActivateType::PREVIEW);
  //Scroll click opens up music player.
  else if (button == 2)
    parent_->Activate(LocalResult(result_), index, ResultView::ActivateType::DIRECT);

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

  ubus_.RegisterInterest(UBUS_DASH_PREVIEW_NAVIGATION_REQUEST, [this] (GVariant* data) {
    int nav_mode = 0;
    GVariant* local_result_variant = nullptr;
    glib::String proposed_unique_id;

    g_variant_get(data, "(ivs)", &nav_mode, &local_result_variant, &proposed_unique_id);
    LocalResult local_result(LocalResult::FromVariant(local_result_variant));
    g_variant_unref(local_result_variant);

    if (proposed_unique_id.Str() != parent_->unique_id())
      return;

    unsigned num_results = coverflow_->model()->Items().size();
    int current_index = parent->GetIndexForLocalResult(local_result);
    if (nav_mode == -1) // left
    {
      current_index--;
    }
    else if (nav_mode == 1) // right
    {
      current_index++;
    }

    if (current_index < 0 || static_cast<unsigned int>(current_index) >= num_results)
    {
      return;
    }

    if (nav_mode)
    {
      parent_->Activate(parent_->GetLocalResultForIndex(current_index), current_index, ActivateType::PREVIEW);
    }
  });
}

CoverflowResultView::Impl::~Impl()
{

}

CoverflowResultView::CoverflowResultView(NUX_FILE_LINE_DECL)
  : ResultView(NUX_FILE_LINE_PARAM)
  , pimpl(new CoverflowResultView::Impl(this))
{
  Style& style = Style::Instance();
  SetMinimumHeight(style.GetTileHeight());
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
  nux::CoverflowItem::Ptr result_item(new CoverflowResultItem(result, this, pimpl->coverflow_->model()));
  pimpl->coverflow_->model()->AddItem(result_item);
}

void CoverflowResultView::RemoveResult(Result& result)
{
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

  if (RedirectedAncestor())
    graphics::ClearGeometry(base);

  if (GetCompositionLayout())
  {
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


void CoverflowResultView::Activate(LocalResult const& local_result, int index, ResultView::ActivateType type)
{
  unsigned num_results = pimpl->coverflow_->model()->Items().size();

  int left_results = index;
  int right_results = num_results ? (num_results - index) - 1 : 0;
  int row_y = GetAbsoluteY();
  int column_x = -1;
  int row_height = renderer_->height;
  int column_width = GetWidth();

  glib::Variant data(g_variant_new("(iiii)", column_x, row_y, column_width, row_height, left_results, right_results));
  ResultActivated.emit(local_result, type, data);
}


}
}
