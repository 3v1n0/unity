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

// Kindly inspired by user_authenticator_linux.cc of chromium project
// In the future would be nice to have something less static. For the moment
// let's just fallcback to lightdm.

#include "UserAuthenticatorPam.h"

#include <cstring>
#include <security/pam_appl.h>

namespace unity
{
namespace lockscreen
{

bool UserAuthenticatorPam::AuthenticateStart(std::string const& username,
                                             std::string const& password,
                                             AuthenticateEndCallback authenticate_cb)
{
  username_ = username;
  password_ = password;
  authenticate_cb_ = authenticate_cb;

  if (!InitPam())
    return false;

  glib::Object<GTask> task(g_task_new(nullptr, cancellable_, [] (GObject*, GAsyncResult*, gpointer data) {
    auto self = static_cast<UserAuthenticatorPam*>(data);
    pam_end(self->pam_handle_, self->status_);
    self->authenticate_cb_(self->status_ == PAM_SUCCESS);
  }, this));

  g_task_set_task_data(task, this, nullptr);

  g_task_run_in_thread(task, [] (GTask* task, gpointer, gpointer data, GCancellable*) {
    auto self = static_cast<UserAuthenticatorPam*>(data);
    self->status_ = pam_authenticate(self->pam_handle_, 0);
  });

  return true;
}

bool UserAuthenticatorPam::InitPam()
{
  pam_conv conversation;
  conversation.conv = ConversationFunction;
  conversation.appdata_ptr = static_cast<void*>(this);

  // FIXME (andy) We should install our own unityshell pam file.
  return pam_start("common-auth", username_.c_str(),
                   &conversation, &pam_handle_) == PAM_SUCCESS;
}

int UserAuthenticatorPam::ConversationFunction(int num_msg,
                                               const pam_message** msg,
                                               pam_response** resp,
                                               void* appdata_ptr)
{
  if (num_msg <= 0)
    return PAM_CONV_ERR;

  auto* tmp_response = static_cast<pam_response*>(malloc(num_msg * sizeof(pam_response)));
  if (!tmp_response)
    return PAM_CONV_ERR;

  UserAuthenticatorPam* user_auth = static_cast<UserAuthenticatorPam*>(appdata_ptr);

  bool raise_error = false;
  int count;
  for (count = 0; count < num_msg && !raise_error; ++count)
  {
    pam_response* resp_item = &tmp_response[count];
    resp_item->resp_retcode = 0;
    resp_item->resp = nullptr;

    switch (msg[count]->msg_style)
    {
      case PAM_PROMPT_ECHO_ON:
        resp_item->resp = strdup(user_auth->username_.c_str());
        if (!resp_item->resp)
          raise_error = true;
        break;
      case PAM_PROMPT_ECHO_OFF:
        resp_item->resp = strdup(user_auth->password_.c_str());
        if (!resp_item->resp)
          raise_error = true;
        break;
      case PAM_TEXT_INFO:
        break;
      default:
        raise_error = true;
    }
  }

  if (raise_error)
  {
    for (int i = 0; i < count; ++i)
      if (tmp_response[i].resp)
        free(tmp_response[i].resp);

    free(tmp_response);
    return PAM_CONV_ERR;
  }
  else
  {
    *resp = tmp_response;
    return PAM_SUCCESS;
  }
}

}  // lockscreen
} // unity
