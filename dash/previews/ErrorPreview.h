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
 *
 */

#ifndef ERROR_PREVIEW_H
#define ERROR_PREVIEW_H

// key used to find the correct info hint
#define ERROR_INFOHINT_ID "error_preview"

// keys of the data preview
#define DATA_TITLE_KEY "title"
#define DATA_HEADER_KEY "header"
#define DATA_SUBTITLE_KEY "subtitle"
#define DATA_PURCHASE_HINT_KEY "purchase_hint"
#define DATA_PURCHASE_PRIZE_KEY "purchase_price"
#define DATA_PURCHASE_TYPE_KEY "purchase_type"
#define DATA_MESSAGE_KEY "message"

// ations ids
#define OPEN_U1_LINK_ACTION "open_u1_link"

#include <Nux/Nux.h>
#include <Nux/AbstractButton.h>
#include <UnityCore/Lens.h>
#include <UnityCore/ErrorPreview.h>
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
  nux::Layout* GetTitle();
  nux::Layout* GetPrize();
  nux::Layout* GetBody();
  nux::Layout* GetFooter();

private:
  std::string GetDataForKey(GVariant *dict, std::string key);
  void LoadActions();

protected:
  void OnActionActivated(ActionButton* button, std::string const& id);
  void OnActionLinkActivated(ActionLink* link, std::string const& id);

  void PreLayoutManagement();

protected:
  virtual void SetupViews();
  // content elements
  nux::ObjectPtr<CoverArt> image_;
  nux::ObjectPtr<nux::StaticCairoText> intro_;
  nux::ObjectPtr<nux::StaticCairoText> title_;
  nux::ObjectPtr<nux::StaticCairoText> subtitle_;
  nux::ObjectPtr<nux::StaticCairoText> purchase_hint_;
  nux::ObjectPtr<nux::StaticCairoText> purchase_prize_;
  nux::ObjectPtr<nux::StaticCairoText> purchase_type_;
  nux::ObjectPtr<nux::HLayout> form_layout_;

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

#endif // ERROR_PREVIEW_H
