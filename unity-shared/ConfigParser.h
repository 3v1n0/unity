/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: Eleni Maria Stea <elenimaria.stea@canonical.com>
 */

#ifndef CONFIG_PARSER_H_
#define CONFIG_PARSER_H_

typedef struct ConfigString ConfigString;

ConfigString *cfgstr_create (const char *str);
void cfgstr_destroy (ConfigString *cfgstr);

const char *cfgstr_get_string(const ConfigString *cfgstr);

const char *cfgstr_get (const ConfigString *cfgstr, const char *key);
float cfgstr_get_float (const ConfigString *cfgstr, const char *key);
int cfgstr_get_int (const ConfigString *cfgstr, const char *key);

int cfgstr_set (ConfigString *cfgstr, const char *key, const char *value);
int cfgstr_set_float (ConfigString *cfgstr, const char *key, float value);
int cfgstr_set_int (ConfigString *cfgstr, const char *key, int value);

#endif // CONFIG_PARSER_H_
