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
 * Authored by: Diego Sarmentero <diego.sarmentero@canonical.com>
 *              Manuel de la Pena <manuel.delapena@canonical.com>
 *
 */

#ifndef PAYMENT_PREVIEW_H
#define PAYMENT_PREVIEW_H

// ations ids
#define CHANGE_PAYMENT_ACTION "change_payment_method"
#define FORGOT_PASSWORD_ACTION "forgot_password"
#define CANCEL_PURCHASE_ACTION "cancel_purchase"
#define PURCHASE_ALBUM_ACTION "purchase_album"

#include <Nux/Nux.h>
#include <Nux/AbstractButton.h>
#include <UnityCore/Lens.h>
#include <UnityCore/PaymentPreview.h>
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

class PaymentPreview : public Preview
{
public:
  typedef nux::ObjectPtr<PaymentPreview> Ptr;
  NUX_DECLARE_OBJECT_TYPE(PaymentPreview, Preview);

  PaymentPreview(dash::Preview::Ptr preview_model);
  ~PaymentPreview();

  virtual nux::Area* FindKeyFocusArea(unsigned int key_symbol,
                                      unsigned long x11_key_code,
                                      unsigned long special_keys_state);
  // From debug::Introspectable
  std::string GetName() const;

  void AddProperties(GVariantBuilder* builder);

  // Create and connect an action link to OnActionLinkActivated
  nux::ObjectPtr<ActionLink> CreateLink(dash::Preview::ActionPtr action);

  // Create and connect an action button OnActioButtonActivated
  nux::ObjectPtr<ActionButton> CreateButton(dash::Preview::ActionPtr action);

private:
  nux::Layout* GetHeader(previews::Style& style, GVariant* data);

protected:

  // Return the title layout (including layout data) to be added to the header
  // NULL is a possible return value.
  virtual nux::Layout* GetTitle();

  // Return the pize layout (including data) to be added to the header
  // NULL is a possible return value.
  virtual nux::Layout* GetPrize();

  // Return layout with the content to show. NULL is a possible return value.
  virtual nux::Layout* GetBody();

  // Return layout with the content to show. NULL is a possible return value.
  virtual nux::Layout* GetFooter();

  // Executed when a link is clicked.
  virtual void OnActionActivated(ActionButton* button, std::string const& id);

  // Executed when a button is clicked.
  virtual void OnActionLinkActivated(ActionLink* link, std::string const& id);

  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void PreLayoutManagement();

  void SetupBackground();
  void SetupViews();

protected:
  nux::VLayout* full_data_layout_;
  nux::Layout* header_layout_;
  nux::Layout* body_layout_;
  nux::Layout* footer_layout_;

  // content elements
  nux::ObjectPtr<CoverArt> image_;

  // do we want to type?
  bool entry_selected_;

  typedef std::unique_ptr<nux::AbstractPaintLayer> LayerPtr;
  LayerPtr details_bg_layer_;
};

}
}
}

#endif // PAYMENT_PREVIEW_H
