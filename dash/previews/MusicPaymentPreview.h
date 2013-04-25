// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012-2013 Canonical Ltd.
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
 * Authored by: Manuel de la Pena <manuel.delapena@canonical.com>
 *
 */

#ifndef MUSIC_PAYMENT_PREVIEW_H
#define MUSIC_PAYMENT_PREVIEW_H

#include <Nux/Nux.h>
#include <Nux/AbstractButton.h>
#include <UnityCore/PaymentPreview.h>
#include "ActionButton.h"
#include "ActionLink.h"
#include "PaymentPreview.h"
#include "unity-shared/IconTexture.h"
#include "unity-shared/TextInput.h"

namespace nux
{
class AbstractPaintLayer;
class StaticCairoText;
class VLayout;
}

namespace unity
{
namespace dash
{
namespace previews
{
class CoverArt;
class PreviewInfoHintWidget;

class MusicPaymentPreview : public PaymentPreview
{
public:
  typedef nux::ObjectPtr<MusicPaymentPreview> Ptr;
  NUX_DECLARE_OBJECT_TYPE(MusicPaymentPreview, Preview);

  MusicPaymentPreview(dash::Preview::Ptr preview_model);

private:
  void LoadActions();

protected:
  // key used to find the correct info hint
  static const std::string DATA_INFOHINT_ID;

  // keys of the data preview
  static const std::string DATA_PASSWORD_KEY;

  // ations ids
  static const std::string CHANGE_PAYMENT_ACTION;
  static const std::string FORGOT_PASSWORD_ACTION;
  static const std::string CANCEL_PURCHASE_ACTION;
  static const std::string PURCHASE_ALBUM_ACTION;

  // From debug::Introspectable
  std::string GetName() const;

  nux::Layout* GetTitle();
  nux::Layout* GetPrice();
  nux::Layout* GetBody();
  nux::Layout* GetFormLabels();
  nux::Layout* GetFormFields();
  nux::Layout* GetFormActions();
  nux::Layout* GetFooter();

  const std::string GetErrorMessage(GVariant *dict);

  void OnActionActivated(ActionButton* button, std::string const& id);
  void OnActionLinkActivated(ActionLink* link, std::string const& id);

  virtual void SetupViews();

  void PreLayoutManagement();

protected:
  // content elements
  nux::ObjectPtr<CoverArt> image_;
  nux::ObjectPtr<StaticCairoText> intro_;
  nux::ObjectPtr<StaticCairoText> title_;
  nux::ObjectPtr<StaticCairoText> subtitle_;
  nux::ObjectPtr<StaticCairoText> email_label_;
  nux::ObjectPtr<StaticCairoText> email_;
  nux::ObjectPtr<StaticCairoText> payment_label_;
  nux::ObjectPtr<StaticCairoText> payment_;
  nux::ObjectPtr<StaticCairoText> password_label_;
  nux::ObjectPtr<TextInput> password_entry_;
  nux::ObjectPtr<StaticCairoText> purchase_hint_;
  nux::ObjectPtr<StaticCairoText> purchase_prize_;
  nux::ObjectPtr<StaticCairoText> purchase_type_;
  nux::ObjectPtr<StaticCairoText> change_payment_;
  nux::ObjectPtr<StaticCairoText> forgotten_password_;
  nux::ObjectPtr<StaticCairoText> error_label_;
  nux::ObjectPtr<nux::HLayout> form_layout_;

  dash::PaymentPreview* payment_preview_model_;
  // do we want to type?
  std::string error_message_;

  // actions
  std::map<std::string, nux::ObjectPtr<nux::AbstractButton>> buttons_map_;

  // lock texture
  nux::ObjectPtr<IconTexture> lock_texture_;

  typedef std::unique_ptr<nux::AbstractPaintLayer> LayerPtr;
  LayerPtr details_bg_layer_;
};

}
}
}

#endif // MUSIC_PAYMENT_PREVIEW_H
