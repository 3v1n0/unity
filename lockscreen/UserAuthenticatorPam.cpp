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
#include "unity-shared/UnitySettings.h"

#include <cstring>
#include <security/pam_appl.h>
#include <vector>

namespace unity
{
namespace lockscreen
{

bool UserAuthenticatorPam::AuthenticateStart(std::string const& username,
                                             AuthenticateEndCallback const& authenticate_cb)
{
  first_prompt_ = true;
  username_ = username;
  authenticate_cb_ = authenticate_cb;
  pam_handle_ = nullptr;

  if (!InitPam() || !pam_handle_)
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

    if (self->status_ == PAM_SUCCESS)
    {
      int status2 = pam_acct_mgmt(self->pam_handle_, 0);

      if (status2 == PAM_NEW_AUTHTOK_REQD)
        status2 = pam_chauthtok(self->pam_handle_, PAM_CHANGE_EXPIRED_AUTHTOK);

      if (unity::Settings::Instance().pam_check_account_type())
        self->status_ = status2;

      pam_setcred(self->pam_handle_, PAM_REINITIALIZE_CRED);
    }
  });

  return true;
}

bool UserAuthenticatorPam::InitPam()
{
  pam_conv conversation;
  conversation.conv = ConversationFunction;
  conversation.appdata_ptr = static_cast<void*>(this);

  return pam_start("unity", username_.c_str(),
                   &conversation, &pam_handle_) == PAM_SUCCESS;
}

int UserAuthenticatorPam::ConversationFunction(int num_msg,
                                               const pam_message** msg,
                                               pam_response** resp,
                                               void* appdata_ptr)
{
  if (num_msg <= 0)
    return PAM_CONV_ERR;

  auto* tmp_response = static_cast<pam_response*>(calloc(num_msg, sizeof(pam_response)));

  if (!tmp_response)
    return PAM_CONV_ERR;

  UserAuthenticatorPam* user_auth = static_cast<UserAuthenticatorPam*>(appdata_ptr);

  if (!user_auth->first_prompt_)
  {
    // Adding a timeout ensures that the signal is emitted in the main thread
    user_auth->source_manager_.AddTimeout(0, [user_auth] { user_auth->clear_prompts.emit(); return false; });
  }

  user_auth->first_prompt_ = false;

  bool raise_error = false;
  int count;
  std::vector<PromiseAuthCodePtr> promises;

  for (count = 0; count < num_msg && !raise_error; ++count)
  {
    switch (msg[count]->msg_style)
    {
      case PAM_PROMPT_ECHO_ON:
      {
        auto promise = std::make_shared<std::promise<std::string>>();
        promises.push_back(promise);

        // Adding a timeout ensures that the signal is emitted in the main thread
        std::string message(msg[count]->msg);
        user_auth->source_manager_.AddTimeout(0, [user_auth, message, promise] { user_auth->echo_on_requested.emit(message, promise); return false; });
        break;
      }
      case PAM_PROMPT_ECHO_OFF:
      {
        auto promise = std::make_shared<std::promise<std::string>>();
        promises.push_back(promise);

        // Adding a timeout ensures that the signal is emitted in the main thread
        std::string message(msg[count]->msg);
        user_auth->source_manager_.AddTimeout(0, [user_auth, message, promise] { user_auth->echo_off_requested.emit(message, promise); return false; });
        break;
      }
      case PAM_TEXT_INFO:
      {
        // Adding a timeout ensures that the signal is emitted in the main thread
        std::string message(msg[count]->msg);
        user_auth->source_manager_.AddTimeout(0, [user_auth, message] { user_auth->message_requested.emit(message); return false; });
        break;
      }
      default:
      {
        // Adding a timeout ensures that the signal is emitted in the main thread
        std::string message(msg[count]->msg);
        user_auth->source_manager_.AddTimeout(0, [user_auth, message] { user_auth->error_requested.emit(message); return false; });
      }
    }
  }

  int i = 0;

  for (auto const& promise : promises)
  {
    auto future = promise->get_future();
    pam_response* resp_item = &tmp_response[i++];
    resp_item->resp_retcode = 0;
    resp_item->resp = strdup(future.get().c_str());

    if (!resp_item->resp)
    {
      raise_error = true;
      break;
    }
  }

  if (raise_error)
  {
    for (int i = 0; i < count; ++i)
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
