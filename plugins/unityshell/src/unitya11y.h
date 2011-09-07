/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Alejandro Piñeiro Iglesias <apinheiro@igalia.com>
 */

#ifndef UNITY_A11Y_H
#define UNITY_A11Y_H

#include <atk/atk.h>

#include <Nux/Nux.h>
#include <Nux/WindowThread.h>
#include <NuxCore/Object.h>

void unity_a11y_preset_environment(void);
void unity_a11y_init(nux::WindowThread* wt);
void unity_a11y_finalize(void);

AtkObject* unity_a11y_get_accessible(nux::Object* object);

gboolean unity_a11y_initialized(void);

/* For unit test purposes */

GHashTable* _unity_a11y_get_accessible_table();

#endif /* UNITY_A11Y_H */
