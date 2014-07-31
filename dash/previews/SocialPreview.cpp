// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Ken VanDine <ken.vandine@canonical.com>
 *
 */

#include "unity-shared/IntrospectableWrappers.h"
#include "unity-shared/PreviewStyle.h"
#include "unity-shared/CoverArt.h"
#include "unity-shared/IconTexture.h"
#include <UnityCore/SocialPreview.h>
#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <Nux/GridHLayout.h>
#include <Nux/Button.h>

#include "config.h"
#include <glib/gi18n-lib.h>

#include "SocialPreview.h"
#include "SocialPreviewContent.h"
#include "SocialPreviewComments.h"
#include "ActionButton.h"
#include "PreviewInfoHintWidget.h"

namespace unity
{
namespace dash
{
namespace previews
{
DECLARE_LOGGER(logger, "unity.dash.preview.social");

namespace
{
  const RawPixel CHILDREN_SPACE = 16_em;
  const RawPixel ICON_CHILDREN_SPACE = 3_em;
  const RawPixel SOCIAL_INFO_CHILDREN_SPACE = 12_em;
}

NUX_IMPLEMENT_OBJECT_TYPE(SocialPreview);

SocialPreview::SocialPreview(dash::Preview::Ptr preview_model)
: Preview(preview_model)
, image_data_layout_(nullptr)
, main_social_info_(nullptr)
, comments_layout_(nullptr)
, social_content_layout_(nullptr)
, social_data_layout_(nullptr)
, social_info_layout_(nullptr)
, social_info_scroll_(nullptr)
, icon_layout_(nullptr)
, actions_layout_(nullptr)
{
  SetupViews();
  UpdateScale(scale);
}

SocialPreview::~SocialPreview()
{
}

void SocialPreview::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  gfx_engine.PushClippingRectangle(base);
  nux::GetPainter().PaintBackground(gfx_engine, base);

  gfx_engine.PopClippingRectangle();
}

void SocialPreview::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

  unsigned int alpha, src, dest = 0;
  gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
  gfx_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);

  gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);

  gfx_engine.PopClippingRectangle();
}

std::string SocialPreview::GetName() const
{
  return "SocialPreview";
}

void SocialPreview::AddProperties(debug::IntrospectionData& introspection)
{
  Preview::AddProperties(introspection);
}

void SocialPreview::SetupViews()
{
  dash::SocialPreview* social_preview_model = dynamic_cast<dash::SocialPreview*>(preview_model_.get());
  if (!social_preview_model)
  {
    LOG_ERROR(logger) << "Could not derive social preview model from given parameter.";
    return;
  }

  previews::Style& style = dash::previews::Style::Instance();

  auto on_mouse_down = [this](int x, int y, unsigned long button_flags, unsigned long key_flags) { this->preview_container_->OnMouseDown(x, y, button_flags, key_flags); };

  image_data_layout_ = new nux::HLayout();
  image_data_layout_->SetSpaceBetweenChildren(style.GetPanelSplitWidth().CP(scale));

  nux::VLayout* social_content_layout_ = new nux::VLayout();
  social_content_layout_->SetSpaceBetweenChildren(CHILDREN_SPACE.CP(scale));

  if (social_preview_model->description.Get().length() > 0)
  {
    content_ = new SocialPreviewContent(social_preview_model->description, NUX_TRACKER_LOCATION);
    content_->request_close().connect([this]() { preview_container_->request_close.emit(); });
    social_content_layout_->AddView(content_.GetPointer(), 1);
  }
  else
  {
    image_ = new CoverArt();
    AddChild(image_.GetPointer());
    UpdateCoverArtImage(image_.GetPointer());
    social_content_layout_->AddView(image_.GetPointer(), 1);
  }

  /////////////////////

    /////////////////////
    // Social Data Panel
    full_data_layout_ = new nux::VLayout();
    full_data_layout_->SetPadding(style.GetDetailsTopMargin().CP(scale), 0, style.GetDetailsBottomMargin().CP(scale), style.GetDetailsLeftMargin().CP(scale));
    full_data_layout_->SetSpaceBetweenChildren(CHILDREN_SPACE.CP(scale));

      /////////////////////
      // Main Social Info
      main_social_info_ = new nux::HLayout();
      main_social_info_->SetSpaceBetweenChildren(style.GetSpaceBetweenIconAndDetails().CP(scale));

        /////////////////////
        // Icon Layout
        icon_layout_ = new nux::VLayout();
        icon_layout_->SetSpaceBetweenChildren(ICON_CHILDREN_SPACE.CP(scale));
        avatar_ = new IconTexture(social_preview_model->avatar() ? g_icon_to_string(social_preview_model->avatar()) : "", MIN(style.GetAvatarAreaWidth().CP(scale), style.GetAvatarAreaHeight().CP(scale)));
        AddChild(avatar_.GetPointer());
        avatar_->SetMinMaxSize(style.GetAvatarAreaWidth().CP(scale), style.GetAvatarAreaHeight().CP(scale));
        avatar_->mouse_click.connect(on_mouse_down);
        icon_layout_->AddView(avatar_.GetPointer(), 0);

        /////////////////////

        /////////////////////
        // Data
        social_data_layout_ = new nux::VLayout();
        social_data_layout_->SetSpaceBetweenChildren(style.GetSpaceBetweenTitleAndSubtitle().CP(scale));

        title_ = new StaticCairoText(preview_model_->title, true, NUX_TRACKER_LOCATION);
        AddChild(title_.GetPointer());
        title_->SetLines(-1);
        title_->SetScale(scale);
        title_->SetFont(style.title_font().c_str());
        title_->mouse_click.connect(on_mouse_down);

        subtitle_ = new StaticCairoText(preview_model_->subtitle, true, NUX_TRACKER_LOCATION);
        AddChild(subtitle_.GetPointer());
        subtitle_->SetFont(style.content_font().c_str());
        subtitle_->SetLines(-1);
        subtitle_->SetScale(scale);
        subtitle_->mouse_click.connect(on_mouse_down);

        social_data_layout_->AddView(title_.GetPointer(), 0);
        social_data_layout_->AddView(subtitle_.GetPointer(), 0);
        social_data_layout_->AddSpace(0, 1);

        // buffer space
        /////////////////////

      main_social_info_->AddLayout(icon_layout_, 0);
      main_social_info_->AddLayout(social_data_layout_, 1);
      /////////////////////

      /////////////////////
      // Details
      auto* social_info = new ScrollView(NUX_TRACKER_LOCATION);
      social_info_scroll_ = social_info;
      social_info->scale = scale();
      social_info->EnableHorizontalScrollBar(false);
      social_info->mouse_click.connect(on_mouse_down);

      social_info_layout_ = new nux::VLayout();
      social_info_layout_->SetSpaceBetweenChildren(SOCIAL_INFO_CHILDREN_SPACE.CP(scale));
      social_info->SetLayout(social_info_layout_);

      if (!preview_model_->GetInfoHints().empty())
      {
        preview_info_hints_ = new PreviewInfoHintWidget(preview_model_, style.GetAvatarAreaWidth());
        AddChild(preview_info_hints_.GetPointer());
        preview_info_hints_->request_close().connect([this]() { preview_container_->request_close.emit(); });
        social_info_layout_->AddView(preview_info_hints_.GetPointer(), 0);
      }
      /////////////////////
      // Comments/Replies
      if (!social_preview_model->GetComments().empty())
      {
        comments_layout_ = new nux::HLayout();
        comments_layout_->SetSpaceBetweenChildren(SOCIAL_INFO_CHILDREN_SPACE.CP(scale));
        std::string tmp_comments_hint = _("Comments");
        tmp_comments_hint += ":";

        comments_hint_ = new StaticCairoText(tmp_comments_hint, true, NUX_TRACKER_LOCATION);
        AddChild(comments_hint_.GetPointer());
        comments_hint_->SetLines(-1);
        comments_hint_->SetScale(scale);
        comments_hint_->SetFont(style.info_hint_bold_font().c_str());
        comments_hint_->SetTextAlignment(StaticCairoText::NUX_ALIGN_RIGHT);
        comments_hint_->mouse_click.connect(on_mouse_down);
        comments_layout_->AddView(comments_hint_.GetPointer(), 0, nux::MINOR_POSITION_START);

        comments_ = new SocialPreviewComments(preview_model_, NUX_TRACKER_LOCATION);
        AddChild(comments_.GetPointer());
        comments_->request_close().connect([this]() { preview_container_->request_close.emit(); });
        comments_layout_->AddView(comments_.GetPointer());
        social_info_layout_->AddView(comments_layout_, 0);
      }

      /////////////////////
      // Actions
      action_buttons_.clear();
      actions_layout_ = BuildGridActionsLayout(preview_model_->GetActions(), action_buttons_);
      actions_layout_->SetLeftAndRightPadding(0, style.GetDetailsRightMargin().CP(scale));
      ///////////////////

    full_data_layout_->AddLayout(main_social_info_, 0, nux::MINOR_POSITION_START);
    full_data_layout_->AddView(social_info, 1, nux::MINOR_POSITION_START);
    //full_data_layout_->AddView(comments_.GetPointer(), 1, nux::MINOR_POSITION_START);

    full_data_layout_->AddLayout(actions_layout_, 0);
    /////////////////////

  image_data_layout_->AddView(social_content_layout_, 0);
  image_data_layout_->AddLayout(full_data_layout_, 1);

  mouse_click.connect(on_mouse_down);

  SetLayout(image_data_layout_);
}

void SocialPreview::PreLayoutManagement()
{
  nux::Geometry geo = GetGeometry();
  previews::Style& style = dash::previews::Style::Instance();

  nux::Geometry geo_content(geo.x, geo.y, style.GetAppImageAspectRatio() * geo.height, geo.height);

  int content_width = geo.width - style.GetPanelSplitWidth().CP(scale)
                                - style.GetDetailsLeftMargin().CP(scale)
                                - style.GetDetailsRightMargin().CP(scale);

  if (content_width - geo_content.width < style.GetDetailsPanelMinimumWidth().CP(scale))
    geo_content.width = std::max(0, content_width - style.GetDetailsPanelMinimumWidth().CP(scale));

  if (content_) { content_->SetMinMaxSize(geo_content.width, geo_content.height); }
  if (image_) { image_->SetMinMaxSize(geo_content.width, geo_content.height); }

  int details_width = std::max(0, content_width - geo_content.width);
  int top_social_info_max_width = std::max(0, details_width - style.GetAppIconAreaWidth().CP(scale) - style.GetSpaceBetweenIconAndDetails().CP(scale));

  if (title_) { title_->SetMaximumWidth(top_social_info_max_width); }
  if (subtitle_) { subtitle_->SetMaximumWidth(top_social_info_max_width); }
  if (comments_) { comments_->SetMaximumWidth(top_social_info_max_width); }
  if (comments_hint_) { comments_hint_->SetMinimumWidth(style.GetInfoHintNameMinimumWidth().CP(scale)); }

  int button_w = CLAMP((details_width - style.GetSpaceBetweenActions().CP(scale)) / 2, 0, style.GetActionButtonMaximumWidth().CP(scale));
  int button_h = style.GetActionButtonHeight().CP(scale);

  for (nux::AbstractButton* button : action_buttons_)
    button->SetMinMaxSize(button_w, button_h);

  Preview::PreLayoutManagement();
}

void SocialPreview::UpdateScale(double scale)
{
  Preview::UpdateScale(scale);

  if (preview_info_hints_)
    preview_info_hints_->scale = scale;

  previews::Style& style = dash::previews::Style::Instance();

  if (avatar_)
  {
    avatar_->SetMinMaxSize(style.GetAvatarAreaWidth().CP(scale), style.GetAvatarAreaHeight().CP(scale));
    avatar_->SetSize(MIN(style.GetAvatarAreaWidth().CP(scale), style.GetAvatarAreaHeight().CP(scale)));
    avatar_->ReLoadIcon();
  }

  if (image_data_layout_)
    image_data_layout_->SetSpaceBetweenChildren(style.GetPanelSplitWidth().CP(scale));

  if (social_content_layout_)
    social_content_layout_->SetSpaceBetweenChildren(CHILDREN_SPACE.CP(scale));

  if (main_social_info_)
    main_social_info_->SetSpaceBetweenChildren(style.GetSpaceBetweenIconAndDetails().CP(scale));

  if (icon_layout_)
    icon_layout_->SetSpaceBetweenChildren(ICON_CHILDREN_SPACE.CP(scale));

  if (social_data_layout_)
    social_data_layout_->SetSpaceBetweenChildren(style.GetSpaceBetweenTitleAndSubtitle().CP(scale));

  if (social_info_layout_)
    social_info_layout_->SetSpaceBetweenChildren(SOCIAL_INFO_CHILDREN_SPACE.CP(scale));

  if (social_info_scroll_)
    social_info_scroll_->scale = scale;

  if (actions_layout_)
    actions_layout_->SetLeftAndRightPadding(0, style.GetDetailsRightMargin().CP(scale));

  if (content_)
    content_->scale = scale;

  if (comments_)
    comments_->scale = scale;

  if (comments_layout_)
    comments_layout_->SetSpaceBetweenChildren(SOCIAL_INFO_CHILDREN_SPACE.CP(scale));

  if (comments_hint_)
    comments_hint_->SetScale(scale);
}

} // namespace previews
} // namespace dash
} // namepsace unity
