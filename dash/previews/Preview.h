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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */

#ifndef PREVIEW_H
#define PREVIEW_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <UnityCore/Preview.h>
#include "unity-shared/Introspectable.h"
#include "unity-shared/PreviewStyle.h"
#include "unity-shared/StaticCairoText.h"

namespace nux
{
class AbstractButton;
class AbstractPaintLayer;
class Layout;
class VLayout;
class StaticCairoText;
}

namespace unity
{
namespace dash
{
class ActionButton;

namespace previews
{
class CoverArt;
class TabIterator;
class PreviewInfoHintWidget;
class PreviewContainer;

class Preview : public nux::View, public debug::Introspectable
{
public:
  typedef nux::ObjectPtr<Preview> Ptr;
  NUX_DECLARE_OBJECT_TYPE(Preview, nux::View);

  Preview(dash::Preview::Ptr preview_model);
  virtual ~Preview();

  // From debug::Introspectable
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  static previews::Preview::Ptr PreviewForModel(dash::Preview::Ptr model);

  sigc::signal<void> request_close() const;

  virtual nux::Area* FindKeyFocusArea(unsigned int key_symbol,
                                      unsigned long x11_key_code,
                                      unsigned long special_keys_state);
  virtual nux::Area* KeyNavIteration(nux::KeyNavDirection direction);

protected:
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {}
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {}

  virtual void OnActionActivated(ActionButton* button, std::string const& id);

  virtual void OnNavigateIn();
  virtual void OnNavigateInComplete() {}
  virtual void OnNavigateOut() {}

  virtual bool AcceptKeyNavFocus() { return false; }

  virtual void SetupViews() = 0;

  nux::Layout* BuildGridActionsLayout(dash::Preview::ActionPtrList actions, std::list<nux::AbstractButton*>& buttons);
  nux::Layout* BuildVerticalActionsLayout(dash::Preview::ActionPtrList actions, std::list<nux::AbstractButton*>& buttons);

  void UpdateCoverArtImage(CoverArt* cover_art);

  void SetFirstInTabOrder(nux::InputArea* area);
  void SetLastInTabOrder(nux::InputArea* area);
  void SetTabOrder(nux::InputArea* area, int index);
  void SetTabOrderBefore(nux::InputArea* area, nux::InputArea* after);
  void SetTabOrderAfter(nux::InputArea* area, nux::InputArea* before);
  void RemoveFromTabOrder(nux::InputArea* area);

protected:
  dash::Preview::Ptr preview_model_;
  std::list<nux::AbstractButton*> action_buttons_;
  TabIterator* tab_iterator_;

  nux::VLayout* full_data_layout_;

  nux::ObjectPtr<CoverArt> image_;
  nux::ObjectPtr<StaticCairoText> title_;
  nux::ObjectPtr<StaticCairoText> subtitle_;
  nux::ObjectPtr<StaticCairoText> description_;
  nux::ObjectPtr<PreviewInfoHintWidget> preview_info_hints_;

  typedef std::unique_ptr<nux::AbstractPaintLayer> LayerPtr;

  friend class PreviewContent;

  // Need to declare this as a pointer to avoid a circular header
  // dependency issue between Preview.h and PreviewContainer.h
  nux::ObjectPtr<PreviewContainer> preview_container_;
};

}
}
}

#endif // PREVIEW_H
