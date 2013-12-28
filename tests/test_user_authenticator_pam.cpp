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

#include <gtest/gtest.h>

#include "lockscreen/UserAuthenticatorPam.h"
#include "test_utils.h"

using unity::lockscreen::UserAuthenticatorPam;

#include <security/pam_appl.h>

// Would be nice to build a testing pam module, but writing a
// pam_authenticate function here works just fine for the moment.
int pam_authenticate(pam_handle_t *pamh, int flags)
{
  pam_conv* conversation;
  struct pam_message msg;
  const struct pam_message *msgp;

  pam_get_item(pamh, PAM_CONV, (const void**) &conversation);

  msg.msg_style = PAM_PROMPT_ECHO_OFF;
  msgp = &msg;

  pam_response* resp = nullptr;
  conversation->conv(1, &msgp, &resp, conversation->appdata_ptr);

  return strcmp(resp[0].resp, "password");
}

namespace
{

struct TestUserAuthenticatorPam : public ::testing::Test
{
  UserAuthenticatorPam user_authenticator_pam_;
};

TEST_F(TestUserAuthenticatorPam, AuthenticateStart)
{
  bool authentication_result = false;

  user_authenticator_pam_.AuthenticateStart("andrea", "password", [&authentication_result](bool success) {
    authentication_result = success;
  });

  Utils::WaitUntilMSec(authentication_result);
}

TEST_F(TestUserAuthenticatorPam, AuthenticateStartWrongPassword)
{
  bool authentication_result = false;

  user_authenticator_pam_.AuthenticateStart("maria", "wrong password", [&authentication_result](bool success) {
    authentication_result = not success;
  });

  Utils::WaitUntilMSec(authentication_result);
}

}
