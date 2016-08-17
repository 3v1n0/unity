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

#ifndef UNITY_LOCKSCREEN_PROMPT_FACTORY
#define UNITY_LOCKSCREEN_PROMPT_FACTORY

#include <NuxCore/NuxCore.h>
#include "UnityCore/SessionManager.h"

namespace unity
{
class MockableBaseWindow;

namespace lockscreen
{
class AbstractUserPromptView;

struct PromptFactory
{
  static nux::ObjectPtr<AbstractUserPromptView> CreatePrompt(session::Manager::Ptr const&);
};

}
}

#endif // UNITY_LOCKSCREEN_PROMPT_FACTORY
