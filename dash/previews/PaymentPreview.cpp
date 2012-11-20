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
#include "PaymentPreview.h"
#include "unity-shared/CoverArt.h"

namespace unity
{

namespace dash
{

namespace previews
{

PaymentPreview::PaymentPreview(dash::Preview::Ptr preview_model)
    : Preview(preview_model)
{
}

PaymentPreview::~PaymentPreview()
{
}


std::string PaymentPreview::GetName() const
{
  return "";
}


void PaymentPreview::AddProperties(GVariantBuilder* builder)
{
}


nux::ObjectPtr<ActionLink> PaymentPreview::CreateLink(dash::Preview::ActionPtr action)
{
  nux::ObjectPtr<ActionLink> link;
  return link;
}


nux::ObjectPtr<ActionButton> PaymentPreview::CreateButton(dash::Preview::ActionPtr action)
{
  nux::ObjectPtr<ActionButton> button;
  return button;
}


void PaymentPreview::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
}

void PaymentPreview::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
}

void PaymentPreview::PreLayoutManagement()
{
}


void PaymentPreview::SetupBackground()
{
}

void PaymentPreview::SetupViews()
{
}

}

}

}
