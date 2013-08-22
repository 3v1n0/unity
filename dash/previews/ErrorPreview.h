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
 * Authored by: Diego Sarmentero <diego.sarmentero@canonical.com>
 *              Manuel de la Pena <manuel.delapena@canonical.com>
 *
 */

#ifndef ERROR_PREVIEW_H
#define ERROR_PREVIEW_H

// key used to find the correct info hint
#define ERROR_INFOHINT_ID "error_preview"

// keys of the data preview
// Necessary??
#define DATA_MESSAGE_KEY "message"

// ations ids
#define OPEN_U1_LINK_ACTION "open_u1_link"

#include <Nux/Nux.h>
#include <Nux/AbstractButton.h>
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

class ErrorPreview : public PaymentPreview
{
public:
  typedef nux::ObjectPtr<ErrorPreview> Ptr;
  NUX_DECLARE_OBJECT_TYPE(ErrorPreview, Preview)

  ErrorPreview(dash::Preview::Ptr preview_model);
  ~ErrorPreview();

  nux::Area* FindKeyFocusArea(unsigned int key_symbol,
                              unsigned long x11_key_code,
                              unsigned long special_keys_state);
  // From debug::Introspectable
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  nux::Layout* GetTitle();
  nux::Layout* GetPrice();
  nux::Layout* GetBody();
  nux::Layout* GetFooter();

private:
  void LoadActions();

protected:

  // ids for the diff actions to have
  static const std::string CANCEL_ACTION;
  static const std::string GO_TO_U1_ACTION;

  void OnActionActivated(ActionButton* button, std::string const& id);
  void OnActionLinkActivated(ActionLink* link, std::string const& id);

  void PreLayoutManagement();

  virtual void SetupViews();
  // content elements
  nux::ObjectPtr<CoverArt> image_;
  nux::ObjectPtr<StaticCairoText> intro_;
  nux::ObjectPtr<StaticCairoText> title_;
  nux::ObjectPtr<StaticCairoText> subtitle_;
  nux::ObjectPtr<StaticCairoText> purchase_hint_;
  nux::ObjectPtr<StaticCairoText> purchase_prize_;
  nux::ObjectPtr<StaticCairoText> purchase_type_;

  dash::PaymentPreview* error_preview_model_;
  // do we want to type?
  bool entry_selected_;

  // actions
  std::map<std::string, nux::ObjectPtr<nux::AbstractButton>> buttons_map_;

  // warning texture
  nux::ObjectPtr<IconTexture> warning_texture_;

  typedef std::unique_ptr<nux::AbstractPaintLayer> LayerPtr;
  LayerPtr details_bg_layer_;
};

}
}
}

#endif // ERROR_PREVIEW_H
