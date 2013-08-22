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

#include "PreviewStyle.h"
#include <NuxCore/Logger.h>

#include <NuxGraphics/GLTextureResourceManager.h>
#include <Nux/PaintLayer.h>

#include <UnityCore/GLibWrapper.h>
#include "config.h"

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

const int preview_width = 770;
const int preview_height = 380;

typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;

template <int default_size = 1>
class LazyLoadTexture
{
public:
  LazyLoadTexture(std::string const& filename)
  : filename_(filename) {}

  nux::BaseTexture* texture(int size = default_size)
  {
    auto tex_iter = textures_.find(size);
    if (tex_iter != textures_.end())
      return tex_iter->second.GetPointer();

    return LoadTexture(size).GetPointer();
  }

private:
  BaseTexturePtr LoadTexture(int size)
  {
    BaseTexturePtr texture;
    std::string full_path = PKGDATADIR + filename_;
    glib::Object<GdkPixbuf> pixbuf;
    glib::Error error;

    pixbuf = ::gdk_pixbuf_new_from_file_at_size(full_path.c_str(), size, size, &error);
    if (error)
    {
      LOG_WARN(logger) << "Unable to texture " << full_path << " at size '" << size << "' : " << error;
    }
    else
    {
      texture.Adopt(nux::CreateTexture2DFromPixbuf(pixbuf, true));
    }
    textures_[size] = texture;
    return texture;
  }
private:
  std::string filename_;
  std::map<int, BaseTexturePtr> textures_;
};

} // namespace


class Style::Impl
{
public:
  Impl(Style* owner)
  : owner_(owner)
  , preview_nav_left_texture_("/preview_previous.svg")
  , preview_nav_right_texture_("/preview_next.svg")
  , preview_play_texture_("/preview_play.svg")
  , preview_pause_texture_("/preview_pause.svg")
  , preview_spin_texture_("/search_spin.svg")
  , warning_icon_texture_("/warning_icon.png")
  {
  }
  ~Impl() {}

  Style* owner_;

  LazyLoadTexture<32> preview_nav_left_texture_;
  LazyLoadTexture<32> preview_nav_right_texture_;
  LazyLoadTexture<32> preview_play_texture_;
  LazyLoadTexture<32> preview_pause_texture_;
  LazyLoadTexture<32> preview_spin_texture_;
  LazyLoadTexture<22> warning_icon_texture_;
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

int Style::GetNavigatorWidth() const
{
  return 42;
}

int Style::GetNavigatorIconSize() const
{
  return 24;  
}

int Style::GetPreviewWidth() const
{
  return preview_width;
}

int Style::GetPreviewHeight() const
{
  return preview_height;
}


int Style::GetPreviewTopPadding() const
{
  return 100;
}

int Style::GetDetailsTopMargin() const
{
  return 5;
}

int Style::GetDetailsBottomMargin() const
{
  return 10;
}

int Style::GetDetailsRightMargin() const
{
  return 10;
}

int Style::GetDetailsLeftMargin() const
{
  return 10;
}

int Style::GetPanelSplitWidth() const
{
  return 10;
}

int Style::GetAppIconAreaWidth() const
{
  return 105;
}

int Style::GetSpaceBetweenTitleAndSubtitle() const
{
  return 6;
}

int Style::GetSpaceBetweenIconAndDetails() const
{
  return 18;
}

int Style::GetTrackHeight() const
{
  return 28;
}

int Style::GetMusicDurationWidth() const
{
  return 40;
}

int Style::GetActionButtonHeight() const
{
  return 34;
}

int Style::GetActionButtonMaximumWidth() const
{
  return 175;
}

int Style::GetSpaceBetweenActions() const
{
  return 10;
}

int Style::GetTrackBarHeight() const
{
  return 25;
}

float Style::GetAppImageAspectRatio() const
{
  return 1.0;
}

int Style::GetDetailsPanelMinimumWidth() const
{
  return 300;
}

int Style::GetInfoHintIconSizeWidth() const
{
  return 24;
}

int Style::GetInfoHintNameMinimumWidth() const
{
  return 100;
}

int Style::GetInfoHintNameMaximumWidth() const
{
  return 160;
}

float Style::GetDescriptionLineSpacing() const
{
  return 2.0;
}

int Style::GetDescriptionLineCount() const
{
  return 20;
}

int Style::GetRatingWidgetHeight() const
{
  return 36;
}

int Style::GetStatusIconSize() const
{
  return 12;
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

int Style::GetPaymentIconAreaWidth() const
{
  return 64;
}

int Style::GetPaymentTextInputHeight() const
{
  return 40;
}

int Style::GetPaymentLockWidth() const
{
  return 22;
}

int Style::GetPaymentLockHeight() const
{
  return 22;
}

int Style::GetPaymentHeaderWidth() const
{
  return 850;
}

int Style::GetPaymentHeaderSpace() const
{
  return 0;
}

int Style::GetPaymentFormSpace() const
{
  return 5;
}

std::string Style::u1_warning_font() const
{
  return "Ubuntu Bold 11.5";
}

float Style::GetVideoImageAspectRatio() const
{
  return float(540)/380;
}

int Style::GetAvatarAreaWidth() const
{
  return 100;
}

int Style::GetAvatarAreaHeight() const
{
  return 100;  
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
  return nux::CreateTexture2DFromFile(
              PKGDATADIR"/lock_icon.png", -1, true);
}

nux::BaseTexture* Style::GetWarningIcon()
{
  return pimpl->warning_icon_texture_.texture();
}

nux::BaseTexture* Style::GetSearchSpinIcon(int size)
{
  return pimpl->preview_spin_texture_.texture(size);
}


} // namespace previews
} // namespace dash
} // namespace unity
