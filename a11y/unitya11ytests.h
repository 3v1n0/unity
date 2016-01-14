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

#ifndef UNITY_A11Y_TESTS_H
#define UNITY_A11Y_TESTS_H

/* For the moment we can't add the a11y tests to the current unity
   tests, as it would require to include the Launcher, that right now
   include symbols only available to a compiz plugin (so not available
   for a standalone app.

   So right now this tests are executed by hand during the developing
   of the accessibility support, but not executed as a standalone test.

   When the Launcher thing became solved (as planned), this tests
   would be moved/adapted to the unity test system */

void unity_run_a11y_unit_tests(void);

#endif /* UNITY_A11Y_H */
