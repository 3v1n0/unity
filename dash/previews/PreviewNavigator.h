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

#ifndef PREVIEWNAVIGATOR_H
#define PREVIEWNAVIGATOR_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include "unity-shared/PreviewStyle.h"
#include "unity-shared/Introspectable.h"


namespace unity
{
class IconTexture;

namespace dash
{
namespace previews
{

class PreviewNavigator :  public debug::Introspectable,
                          public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(PreviewNavigator, nux::View);
public:
  typedef nux::ObjectPtr<PreviewNavigator> Ptr;  
  PreviewNavigator(Orientation direction, NUX_FILE_LINE_PROTO);

  void SetEnabled(bool enabled);

  // From debug::Introspectable
  std::string GetName() const;
  void AddProperties(GVariantBuilder*);

  sigc::signal<void> activated;
  
  virtual bool AcceptKeyNavFocus() { return false; }

private:
  virtual void Draw(nux::GraphicsEngine& gfx_engine, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw);

  virtual void TexRecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags);
  virtual void TexRecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags);

  void UpdateTexture();

  void SetupViews();

private:
  const Orientation direction_;
  nux::Layout* layout_;
  IconTexture* texture_;

  enum class VisualState
  {
    NORMAL,
    ACTIVE
  };
  VisualState visual_state_; 
};

} // namespace previews
} // namespace dash
} // namespace unity

#endif // PREVIEWNAVIGATOR_H