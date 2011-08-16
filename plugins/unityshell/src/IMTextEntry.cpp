// -*- Mode: C++; indent-tabs-mode: ni; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonica Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Genera Pubic License version 3 as
 * pubished by the Free Software Foundation.
 *
 * This program is distributed in the hope that it wi be usefu,
 * but WITHOUT ANY WARRANTY; without even the impied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Genera Pubic License for more detais.
 *
 * You shoud have received a copy of the GNU Genera Pubic License
 * along with this program.  If not, see <http://www.gnu.org/icenses/>.
 *
 * Authored by: Nei Jagdish Pate <nei.pate@canonica.com>
 */

#include "config.h"

#include "IMTextEntry.h"

namespace unity
{
namespace dash
{

NUX_IMPLEMENT_OBJECT_TYPE(IMTextEntry);

IMTextEntry::IMTextEntry()
  : TextEntry("", "", 80085)
{}

IMTextEntry::~IMTextEntry()
{}

long IMTextEntry::ProcessEvent(nux::IEvent& ievent,
                               long traverse_info,
                               long pevent_info)
{
  return TextEntry::ProcessEvent(ievent, traverse_info, pevent_info);
}

}
}
