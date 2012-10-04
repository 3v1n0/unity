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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#ifndef U1PAYMENTPREVIEW_H
#define U1PAYMENTPREVIEW_H

// kye used to find the correct infor hint
#define DATA_INFOHINT_ID "album_purchase_preview"

// keys of the data preview
#define DATA_TITLE_KEY "title"
#define DATA_SUBTITLE_KEY "subtitle"
#define DATA_HEADER_KEY "header"
#define DATA_EMAIL_LABEL_KEY "email_label"
#define DATA_EMAIL_KEY "email"
#define DATA_PAYMENT_LABEL_KEY "payment_label"
#define DATA_PAYMENT_METHOD_KEY "payment_method"
#define DATA_PASSWORD_LABEL_KEY "password_label"
#define DATA_PASSWORD_HINT_KEY "password_hint"
#define DATA_PURCHASE_HINT_KEY "purchase_hint"
#define DATA_PURCHASE_PRIZE_KEY "purchase_price"
#define DATA_PURCHASE_TYPE_KEY "purchase_type"
#define DATA_PASSWORD_KEY "password"

// ations ids
#define CHANGE_PAYMENT_ACTION "change_payment_method"
#define FORGOT_PASSWORD_ACTION "forgot_password"
#define CANCEL_PURCHASE_ACTION "cancel_purchase"
#define PURCHASE_ALBUM_ACTION "purchase_album"

#include <Nux/Nux.h>
#include <Nux/AbstractButton.h>
#include <UnityCore/Lens.h>
#include <UnityCore/U1PaymentPreview.h>
#include "ActionButton.h"
#include "ActionLink.h"
#include "Preview.h"
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

class U1PaymentPreview : public Preview
{
public:
  typedef nux::ObjectPtr<U1PaymentPreview> Ptr;
  NUX_DECLARE_OBJECT_TYPE(U1PaymentPreview, Preview);

  U1PaymentPreview(dash::Preview::Ptr preview_model);
  ~U1PaymentPreview();

  virtual nux::Area* FindKeyFocusArea(unsigned int key_symbol,
                                      unsigned long x11_key_code,
                                      unsigned long special_keys_state);
  // From debug::Introspectable
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  std::string GetDataForKey(GVariant *dict, std::string key);
  void LoadActions();
  nux::Layout* GetHeader(previews::Style& style, GVariant* data);
  nux::Layout* GetTitle(previews::Style& style, GVariant* data);
  nux::Layout* GetPrize(previews::Style& style, GVariant* data );
  nux::Layout* GetForm(previews::Style& style, GVariant* data);
  nux::Layout* GetFormLabels(previews::Style& style, GVariant* data);
  nux::Layout* GetFormFields(previews::Style& style, GVariant* data);
  nux::Layout* GetFormActions(previews::Style& style, GVariant* data);
  nux::Layout* GetEmail(previews::Style& style, GVariant* data);
  nux::Layout* GetPayment(previews::Style& style, GVariant* data);
  nux::Layout* GetPassword(previews::Style& style, GVariant* data);
  nux::Layout* GetActions(previews::Style& style, GVariant* data);

protected:
  virtual void OnActionActivated(ActionButton* button, std::string const& id);
  virtual void OnActionLinkActivated(ActionLink* link, std::string const& id);

  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void PreLayoutManagement();

  void SetupBackground();
  void SetupViews();

protected:
  nux::VLayout* full_data_layout_;

  // content elements
  nux::ObjectPtr<CoverArt> image_;
  nux::ObjectPtr<nux::StaticCairoText> title_;
  nux::ObjectPtr<nux::StaticCairoText> subtitle_;
  nux::ObjectPtr<nux::StaticCairoText> header_;
  nux::ObjectPtr<nux::StaticCairoText> email_label_;
  nux::ObjectPtr<nux::StaticCairoText> email_;
  nux::ObjectPtr<nux::StaticCairoText> payment_label_;
  nux::ObjectPtr<nux::StaticCairoText> payment_;
  nux::ObjectPtr<nux::StaticCairoText> password_label_;
  nux::ObjectPtr<TextInput> password_entry_;
  nux::ObjectPtr<nux::StaticCairoText> purchase_hint_;
  nux::ObjectPtr<nux::StaticCairoText> purchase_prize_;
  nux::ObjectPtr<nux::StaticCairoText> purchase_type_;
  nux::ObjectPtr<nux::StaticCairoText> change_payment_;
  nux::ObjectPtr<nux::StaticCairoText> forgotten_password_;

  // do we want to type?
  bool entry_selected_;

  // actions
  std::map<std::string, nux::ObjectPtr<nux::AbstractButton>> sorted_buttons_;

  // lock texture
  IconTexture* lock_texture_;

  typedef std::unique_ptr<nux::AbstractPaintLayer> LayerPtr;
  LayerPtr details_bg_layer_;
};

}
}
}

#endif // U1PAYMENTPREVIEW_H
