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
#include <UnityCore/GLibWrapper.h>
#include "config.h"

namespace unity
{
namespace dash
{
namespace previews
{
namespace
{
Style* style_instance = nullptr;

nux::logging::Logger logger("unity.dash.previews.style");

typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;

class LazyLoadTexture
{
public:
  LazyLoadTexture(std::string const& filename, int size = -1);
  nux::BaseTexture* texture();
private:
  void LoadTexture();
private:
  std::string filename_;
  int size_;
  BaseTexturePtr texture_;
};

} // namespace


class Style::Impl
{
public:
  Impl(Style* owner)
  : owner_(owner)
  , preview_nav_left_texture_("/preview_previous.svg", 32)
  , preview_nav_right_texture_("/preview_next.svg", 32)
  , preview_play_texture_("/preview_play.svg", 32)
  , preview_pause_texture_("/preview_pause.svg", 32)
  {
  }
  ~Impl() {}

  Style* owner_;

  LazyLoadTexture preview_nav_left_texture_;
  LazyLoadTexture preview_nav_right_texture_;
  LazyLoadTexture preview_play_texture_;
  LazyLoadTexture preview_pause_texture_;
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

int Style::GetNavigatorWidth() const
{
  return 42;
}

int Style::GetNavigatorIconSize() const
{
  return 24;  
}

int Style::GetPreviewPreferredHeight() const
{
  return 390;
}

int Style::GetPreviewPreferredWidth() const
{
  return 796;
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
  return 16;
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
  return 36;
}

int Style::GetActionButtonMaximumWidth() const
{
  return 190;
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

int Style::GetInfoHintNameWidth() const
{
  return 100;
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

float Style::GetVideoImageAspectRatio() const
{
  return float(540)/380;
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



namespace
{
LazyLoadTexture::LazyLoadTexture(std::string const& filename, int size)
  : filename_(filename)
  , size_(size)
{
}

nux::BaseTexture* LazyLoadTexture::texture()
{
  if (!texture_)
    LoadTexture();
  return texture_.GetPointer();
}

void LazyLoadTexture::LoadTexture()
{
  std::string full_path = PKGDATADIR + filename_;
  glib::Object<GdkPixbuf> pixbuf;
  glib::Error error;

  pixbuf = ::gdk_pixbuf_new_from_file_at_size(full_path.c_str(), size_, size_, &error);
  if (error)
  {
    LOG_WARN(logger) << "Unable to texture " << full_path << ": " << error;
  }
  else
  {
    texture_.Adopt(nux::CreateTexture2DFromPixbuf(pixbuf, true));
  }
}

} // namesspace

} // namespace previews
} // namespace dash
} // namespace unity
