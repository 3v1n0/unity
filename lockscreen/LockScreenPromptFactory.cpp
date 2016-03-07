// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2015 Canonical Ltd
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 3 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Authored by: Marco Trevisan <marco.trevisan@canonical.com>
*/

#include "LockScreenPromptFactory.h"
#include "KylinUserPromptView.h"
#include "UserPromptView.h"
#include "unity-shared/UnitySettings.h"

namespace unity
{
namespace lockscreen
{
nux::ObjectPtr<AbstractUserPromptView> PromptFactory::CreatePrompt(session::Manager::Ptr const& sm)
{
  nux::ObjectPtr<AbstractUserPromptView> prompt;

  if (unity::Settings::Instance().desktop_type() == DesktopType::UBUNTUKYLIN)
    prompt = new KylinUserPromptView(sm);
  else
    prompt = new UserPromptView(sm);

  return prompt;
}

}
}
