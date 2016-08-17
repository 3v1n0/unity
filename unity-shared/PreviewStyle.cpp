// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2011 Canonical Ltd.
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#include "config.h"

#include "PreviewStyle.h"
#include <NuxCore/Logger.h>
#include <Nux/PaintLayer.h>

#include "unity-shared/ThemeSettings.h"

namespace unity
{
namespace dash
{
namespace previews
{
DECLARE_LOGGER(logger, "unity.dash.previews.style");
namespace
{
Style* style_instance = nullptr;

const RawPixel preview_width = 770_em;
const RawPixel preview_height = 380_em;

typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;

template <int default_size = -1>
class LazyLoadTexture
{
public:
  LazyLoadTexture(std::string const& texture_name)
    : texture_name_(texture_name)
  {}

  nux::BaseTexture* texture()
  {
    if (texture_)
      return texture_.GetPointer();

    return LoadTexture().GetPointer();
  }

private:
  BaseTexturePtr LoadTexture()
  {
    auto const& texture_path = theme::Settings::Get()->ThemedFilePath(texture_name_, {PKGDATADIR});
    texture_.Release();

    if (!texture_path.empty())
      texture_.Adopt(nux::CreateTexture2DFromFile(texture_path.c_str(), default_size, true));

    return texture_;
  }

  std::string texture_name_;
  BaseTexturePtr texture_;
};

} // namespace


class Style::Impl
{
public:
  Impl(Style* owner)
  : owner_(owner)
  , preview_nav_left_texture_("preview_previous")
  , preview_nav_right_texture_("preview_next")
  , preview_play_texture_("preview_play")
  , preview_pause_texture_("preview_pause")
  , warning_icon_texture_("warning_icon")
  , lock_icon_("lock_icon")
  {}

  Style* owner_;

  LazyLoadTexture<32> preview_nav_left_texture_;
  LazyLoadTexture<32> preview_nav_right_texture_;
  LazyLoadTexture<32> preview_play_texture_;
  LazyLoadTexture<32> preview_pause_texture_;
  LazyLoadTexture<22> warning_icon_texture_;
  LazyLoadTexture<-1> lock_icon_;
};


Style::Style()
: pimpl(new Impl(this))
{
  if (style_instance)
  {
    LOG_ERROR(logger) << "More than one previews::Style created.";
  }
  else
  {
    style_instance = this;
  }
}

Style::~Style()
{
  if (style_instance == this)
    style_instance = nullptr;
}

Style& Style::Instance()
{
  if (!style_instance)
  {
    LOG_ERROR(logger) << "No previews::Style created yet.";
  }

  return *style_instance;
}

nux::AbstractPaintLayer* Style::GetBackgroundLayer() const
{
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  return new nux::ColorLayer(nux::Color(0.0f, 0.0f, 0.0f, 0.1f), true, rop);
}

RawPixel Style::GetNavigatorWidth() const
{
  return 42_em;
}

RawPixel Style::GetNavigatorIconSize() const
{
  return 24_em;
}

RawPixel Style::GetPreviewWidth() const
{
  return preview_width;
}

RawPixel Style::GetPreviewHeight() const
{
  return preview_height;
}

RawPixel Style::GetPreviewTopPadding() const
{
  return 100_em;
}

RawPixel Style::GetDetailsTopMargin() const
{
  return 5_em;
}

RawPixel Style::GetDetailsBottomMargin() const
{
  return 10_em;
}

RawPixel Style::GetDetailsRightMargin() const
{
  return 10_em;
}

RawPixel Style::GetDetailsLeftMargin() const
{
  return 10_em;
}

RawPixel Style::GetPanelSplitWidth() const
{
  return 10_em;
}

RawPixel Style::GetAppIconAreaWidth() const
{
  return 105_em;
}

RawPixel Style::GetSpaceBetweenTitleAndSubtitle() const
{
  return 6_em;
}

RawPixel Style::GetSpaceBetweenIconAndDetails() const
{
  return 18_em;
}

RawPixel Style::GetTrackHeight() const
{
  return 28_em;
}

RawPixel Style::GetMusicDurationWidth() const
{
  return 40_em;
}

RawPixel Style::GetActionButtonHeight() const
{
  return 34_em;
}

RawPixel Style::GetActionButtonMaximumWidth() const
{
  return 175_em;
}

RawPixel Style::GetSpaceBetweenActions() const
{
  return 10_em;
}

RawPixel Style::GetTrackBarHeight() const
{
  return 25_em;
}

float Style::GetAppImageAspectRatio() const
{
  return 1.0;
}

RawPixel Style::GetDetailsPanelMinimumWidth() const
{
  return 300_em;
}

RawPixel Style::GetInfoHintIconSizeWidth() const
{
  return 24_em;
}

RawPixel Style::GetInfoHintNameMinimumWidth() const
{
  return 100_em;
}

RawPixel Style::GetInfoHintNameMaximumWidth() const
{
  return 160_em;
}

float Style::GetDescriptionLineSpacing() const
{
  return 2.0;
}

RawPixel Style::GetDescriptionLineCount() const
{
  return 20_em;
}

RawPixel Style::GetRatingWidgetHeight() const
{
  return 36_em;
}

RawPixel Style::GetStatusIconSize() const
{
  return 10_em;
}

std::string Style::payment_title_font() const
{
  return "Ubuntu 22";
}

std::string Style::payment_subtitle_font() const
{
  return "Ubuntu 12.5";
}

std::string Style::payment_prize_title_font() const
{
  return "Ubuntu Bold 12.5";
}

std::string Style::payment_prize_subtitle_font() const
{
  return "Ubuntu 10";
}

std::string Style::payment_intro_font() const
{
  return "Ubuntu 11.7";
}

std::string Style::payment_form_labels_font() const
{
  return "Ubuntu 10";
}

std::string Style::payment_form_data_font() const
{
  return "Ubuntu Bold 10";
}

std::string Style::payment_form_actions_font() const
{
  return "Ubuntu 10";
}

std::string Style::payment_text_input_font() const
{
  return "Ubuntu 14";
}

nux::Color Style::payment_error_color() const
{
  return nux::Color(255, 0, 0);
}

RawPixel Style::GetPaymentIconAreaWidth() const
{
  return 64_em;
}

RawPixel Style::GetPaymentTextInputHeight() const
{
  return 40_em;
}

RawPixel Style::GetPaymentLockWidth() const
{
  return 22_em;
}

RawPixel Style::GetPaymentLockHeight() const
{
  return 22_em;
}

RawPixel Style::GetPaymentHeaderWidth() const
{
  return 850_em;
}

RawPixel Style::GetPaymentHeaderSpace() const
{
  return 0_em;
}

RawPixel Style::GetPaymentFormSpace() const
{
  return 5_em;
}

std::string Style::u1_warning_font() const
{
  return "Ubuntu Bold 11.5";
}

float Style::GetVideoImageAspectRatio() const
{
  return float(540)/380;
}

RawPixel Style::GetAvatarAreaWidth() const
{
  return 100_em;
}

RawPixel Style::GetAvatarAreaHeight() const
{
  return 100_em;
}

std::string Style::content_font() const
{
  return "Ubuntu Light 12";
}

std::string Style::title_font() const
{
  return "Ubuntu 22";
}

std::string Style::subtitle_size_font() const
{
  return "Ubuntu 12.5";
}

std::string Style::description_font() const
{
  return "Ubuntu Light 10";

}
std::string Style::action_font() const
{
  return "Ubuntu 11";
}
std::string Style::action_extra_font() const
{
  return "Ubuntu Bold 11";
}

std::string Style::app_license_font() const
{
  return "Ubuntu Light 10";
}

std::string Style::app_last_update_font() const
{
  return "Ubuntu Light 10";
}

std::string Style::app_copywrite_font() const
{
  return "Ubuntu Light 10";
}

std::string Style::info_hint_font() const
{
  return "Ubuntu Light 10";
}

std::string Style::info_hint_bold_font() const
{
  return "Ubuntu Bold 10";
}

std::string Style::user_rating_font() const
{
  return "Ubuntu Light 10";
}

std::string Style::no_preview_image_font() const
{
  return "Ubuntu Light 16";
}

std::string Style::track_font() const
{
  return "Ubuntu Light 10";
}

bool Style::GetShadowBackgroundEnabled() const
{
  return false;
}

nux::BaseTexture* Style::GetNavLeftIcon()
{
  return pimpl->preview_nav_left_texture_.texture();
}

nux::BaseTexture* Style::GetNavRightIcon()
{
  return pimpl->preview_nav_right_texture_.texture();
}

nux::BaseTexture* Style::GetPlayIcon()
{
  return pimpl->preview_play_texture_.texture();
}

nux::BaseTexture* Style::GetPauseIcon()
{
  return pimpl->preview_pause_texture_.texture();
}

nux::BaseTexture* Style::GetLockIcon()
{
  return pimpl->lock_icon_.texture();
}

nux::BaseTexture* Style::GetWarningIcon()
{
  return pimpl->warning_icon_texture_.texture();
}


} // namespace previews
} // namespace dash
} // namespace unity
