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

#ifndef PREVIEWSTYLE_H
#define PREVIEWSTYLE_H

#include <Nux/Nux.h>
#include <Nux/View.h>

#include <string>
#include <memory>

#include "unity-shared/RawPixel.h"

namespace nux
{
  class BaseTexture;
  class AbstractPaintLayer;
}

namespace unity
{
namespace dash
{
namespace previews
{

enum class Orientation {
  UP,
  DOWN,
  LEFT,
  RIGHT
};

class Style
{
public:
  Style();
  ~Style();

  static Style& Instance();

  RawPixel GetNavigatorWidth() const;
  RawPixel GetNavigatorIconSize() const;

  RawPixel GetPreviewWidth() const;
  RawPixel GetPreviewHeight() const;
  RawPixel GetPreviewTopPadding() const;

  RawPixel GetDetailsTopMargin() const;
  RawPixel GetDetailsBottomMargin() const;
  RawPixel GetDetailsRightMargin() const;
  RawPixel GetDetailsLeftMargin() const;
  RawPixel GetPanelSplitWidth() const;

  RawPixel GetSpaceBetweenTitleAndSubtitle() const;

  RawPixel GetActionButtonHeight() const;
  RawPixel GetSpaceBetweenActions() const;
  RawPixel GetActionButtonMaximumWidth() const;

  RawPixel GetDetailsPanelMinimumWidth() const;

  RawPixel GetInfoHintIconSizeWidth() const;
  RawPixel GetInfoHintNameMinimumWidth() const;
  RawPixel GetInfoHintNameMaximumWidth() const;

  RawPixel GetCommentNameMinimumWidth() const;
  RawPixel GetCommentNameMaximumWidth() const;

  float GetDescriptionLineSpacing() const;
  RawPixel GetDescriptionLineCount() const;

  RawPixel GetRatingWidgetHeight() const;

  bool GetShadowBackgroundEnabled() const;

  std::string title_font() const;
  std::string subtitle_size_font() const;
  std::string description_font() const;

  std::string action_font() const;
  std::string action_extra_font() const;

  nux::AbstractPaintLayer* GetBackgroundLayer() const;

  ////////////////////////////////
  // Application Preview
  std::string app_license_font() const;
  std::string app_last_update_font() const;
  std::string app_copywrite_font() const;
  std::string info_hint_font() const;
  std::string info_hint_bold_font() const;
  std::string user_rating_font() const;
  std::string no_preview_image_font() const;

  float GetAppImageAspectRatio() const;

  RawPixel GetAppIconAreaWidth() const;
  RawPixel GetSpaceBetweenIconAndDetails() const;
   ////////////////////////////////

  ////////////////////////////////
  // Music Preview
  std::string track_font() const;
  std::string u1_warning_font() const;
  nux::BaseTexture* GetWarningIcon();

  RawPixel GetTrackHeight() const;
  ////////////////////////////////

  ////////////////////////////////
  // Movie Preview
  float GetVideoImageAspectRatio() const;

  RawPixel GetTrackBarHeight() const;
  RawPixel GetMusicDurationWidth() const;
  RawPixel GetStatusIconSize() const;
  ////////////////////////////////

  ////////////////////////////////
  // Social Preview
  RawPixel GetAvatarAreaWidth() const;
  RawPixel GetAvatarAreaHeight() const;

  std::string content_font() const;

  ////////////////////////////////

  nux::BaseTexture* GetNavLeftIcon();
  nux::BaseTexture* GetNavRightIcon();
  nux::BaseTexture* GetPlayIcon();
  nux::BaseTexture* GetPauseIcon();
  nux::BaseTexture* GetLockIcon();

  ////////////////////////////////
  // Payment Preview
  std::string payment_title_font() const;
  std::string payment_subtitle_font() const;
  std::string payment_prize_title_font() const;
  std::string payment_prize_subtitle_font() const;
  std::string payment_intro_font() const;
  std::string payment_form_labels_font() const;
  std::string payment_form_data_font() const;
  std::string payment_form_actions_font() const;
  std::string payment_text_input_font() const;
  nux::Color payment_error_color() const;


  RawPixel GetPaymentIconAreaWidth() const;
  RawPixel GetPaymentTextInputHeight() const;
  RawPixel GetPaymentLockWidth() const;
  RawPixel GetPaymentLockHeight() const;
  RawPixel GetPaymentHeaderWidth() const;
  RawPixel GetPaymentHeaderSpace() const;
  RawPixel GetPaymentFormSpace() const;

  /////////////////////////////////

protected:
  class Impl;
  std::unique_ptr<Impl> pimpl;

};

}
}
}

#endif //PREVIEWSTYLE_H
