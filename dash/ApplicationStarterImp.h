/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#ifndef UNITY_SHARED_APPLICATION_STARTER_IMP_H
#define UNITY_SHARED_APPLICATION_STARTER_IMP_H

#include "ApplicationStarter.h"

namespace unity {

class ApplicationStarterImp : public ApplicationStarter 
{
public:
  bool Launch(std::string const& application_name, Time timestamp) override;
};

}

#endif
