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
#include <Nux/HLayout.h>
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

  PaymentPreview(dash::Preview::Ptr preview_model);
  ~PaymentPreview();

  // From debug::Introspectable
  std::string GetName() const;

  void AddProperties(GVariantBuilder* builder);

  // Create and connect an action link to OnActionLinkActivated
  nux::ObjectPtr<ActionLink> CreateLink(dash::Preview::ActionPtr action);

  // Create and connect an action button OnActioButtonActivated
  nux::ObjectPtr<ActionButton> CreateButton(dash::Preview::ActionPtr action);

  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

private:
  nux::Layout* GetHeader();

protected:
  GVariant *data_;
  // Return the title layout (including layout data) to be added to the header
  // NULL is a possible return value.
  virtual nux::Layout* GetTitle() = 0;

  // Return the pize layout (including data) to be added to the header
  // NULL is a possible return value.
  virtual nux::Layout* GetPrize() = 0;

  // Return layout with the content to show. NULL is a possible return value.
  virtual nux::Layout* GetBody() = 0;

  // Return layout with the content to show. NULL is a possible return value.
  virtual nux::Layout* GetFooter() = 0;

  // Executed when a link is clicked.
  virtual void OnActionActivated(ActionButton* button, std::string const& id) = 0;

  // Executed when a button is clicked.
  virtual void OnActionLinkActivated(ActionLink* link, std::string const& id) = 0;

  virtual void PreLayoutManagement() = 0;

  virtual void LoadActions() = 0;
  virtual void SetupViews();
  virtual void SetupBackground();

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
