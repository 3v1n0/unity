// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#ifndef UNITY_USER_AUTHENTICATOR_PAM_H
#define UNITY_USER_AUTHENTICATOR_PAM_H

#include <boost/noncopyable.hpp>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSource.h>

#include "UserAuthenticator.h"

// Forward declarations
struct pam_handle;
struct pam_message;
struct pam_response;

namespace unity
{
namespace lockscreen
{

class UserAuthenticatorPam : public UserAuthenticator, private boost::noncopyable
{
public:
  bool AuthenticateStart(std::string const& username, AuthenticateEndCallback const&) override;

private:
  // TODO (andy) move to pimpl
  bool InitPam();

  static int ConversationFunction(int num_msg,
                                  const pam_message** msg,
                                  pam_response** resp,
                                  void* appdata_ptr);

  std::string username_;
  AuthenticateEndCallback authenticate_cb_;

  int status_;
  bool first_prompt_;
  pam_handle* pam_handle_;
  glib::Cancellable cancellable_;
  glib::SourceManager source_manager_;
};

} // lockscreen
} // unity

#endif // UNITY_USER_AUTHENTICATOR_PAM_H
