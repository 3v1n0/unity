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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ConfigParser.h"

static char *strip_whitespace (char *buf);

/* Linked List */

struct Node {
  char *key;
  char *value;

  struct Node *next;
};

/* ConfigString */

struct ConfigString {
  struct Node *list;
  char *string;
};

ConfigString *cfgstr_create (const char *orig_str)
{
  char *sptr, *str;

  str = (char*)alloca(strlen(orig_str) + 1);
  strcpy(str, orig_str);

  ConfigString *cfgstr;
  if (!(cfgstr = (ConfigString*)malloc(sizeof *cfgstr)))
    return 0;

  cfgstr->list = NULL;
  cfgstr->string = NULL;

  sptr = str;
  while (sptr && *sptr != 0)
  {
    char *key, *value, *end, *start, *vstart;
    struct Node *node;

    if (*sptr == ';')
    {
      sptr++;
      continue;
    }

    start = sptr;
    if ((end = strchr(start, ';')))
    {
      *end = 0;
      sptr = end + 1;
    }
    else
    {
      sptr = NULL;
    }

    /* parse key/value from the string */
    if (!(vstart = strchr(start, '=')))
    {
      fprintf(stderr, "%s: invalid key-value pair: %s\n", __func__, start);
      cfgstr_destroy(cfgstr);
      return 0;
    }
    *vstart++ = 0;  /* terminate the key part and move the pointer to the start of the value */

    start = strip_whitespace(start);
    vstart = strip_whitespace(vstart);

    if (!(key = (char*)malloc(strlen(start) + 1)))
    {
      cfgstr_destroy(cfgstr);
      return 0;
    }

    if (!(value = (char*)malloc(strlen(vstart) + 1)))
    {
      free(key);
      cfgstr_destroy(cfgstr);
      return 0;
    }

    strcpy(key, start);
    strcpy(value, vstart);

    /* create new list node and add to the list */
    if (!(node = (Node*)malloc(sizeof *node)))
    {
      free(key);
      free(value);
      cfgstr_destroy(cfgstr);
      return 0;
    }

    node->key = key;
    node->value = value;
    node->next = cfgstr->list;
    cfgstr->list = node;
  }

  return cfgstr;
}

void cfgstr_destroy (ConfigString *cfgstr)
{
  if (!cfgstr)
    return;

  while (cfgstr->list)
  {
    struct Node *node = cfgstr->list;
    cfgstr->list = cfgstr->list->next;
    free(node->key);
    free(node->value);
    free(node);
  }
  free(cfgstr->string);
  free(cfgstr);
}

const char *cfgstr_get_string (const ConfigString *cfgstr)
{
  int len;
  const struct Node *node;
  char *end;

  free(cfgstr->string);

  /* determine the string size */
  len = 0;
  node = cfgstr->list;
  while (node)
  {
    len += strlen(node->key) + strlen(node->value) + 2;
    node = node->next;
  }

  if (!(((ConfigString*)cfgstr)->string = (char*)malloc(len + 1)))
    return 0;

  end = cfgstr->string;
  node = cfgstr->list;

  while (node)
  {
    end += sprintf(end, "%s=%s;", node->key, node->value);
    node = node->next;
  }

  return cfgstr->string;
}

static struct Node *find_node(struct Node *node, const char *key)
{
  while (node)
  {
    if (strcmp(node->key, key) == 0)
      return node;

    node = node->next;
  }
  return 0;
}

const char *cfgstr_get (const ConfigString *cfgstr, const char *key)
{
  struct Node *node = find_node(cfgstr->list, key);
  if (!node)
    return 0;

  return node->value;
}

float cfgstr_get_float (const ConfigString *cfgstr, const char *key)
{
  const char *val_str = cfgstr_get(cfgstr, key);
  return val_str ? atof(val_str) : 0;
}

int cfgstr_get_int (const ConfigString *cfgstr, const char *key)
{
  const char *val_str = cfgstr_get(cfgstr, key);
  return val_str ? atoi(val_str) : 0;
}

/*
 * returns:
 * -1: on error
 *  0: successfuly updated value
 *  1: added new key-value pair
 * */

int cfgstr_set (ConfigString *cfgstr, const char *key, const char *value)
{
  char *new_val;
  struct Node *node = find_node(cfgstr->list, key);
  if (!node)
  {
    if ((!(node = (Node*)malloc(sizeof *node))))
      return -1;

    if ((!(node->key = (char*)malloc(strlen(key) + 1))))
    {
      free(node);
      return -1;
    }

    if ((!(node->value = (char*)malloc(strlen(value) + 1))))
    {
      free(node->key);
      free(node);
      return -1;
    }

    strcpy(node->key, key);
    strcpy(node->value, value);
    node->next = cfgstr->list;
    cfgstr->list = node;

    return 1;
  }

  if ((!(new_val = (char*)malloc(strlen(value) + 1))))
    return -1;

  strcpy(new_val, value);
  free(node->value);
  node->value = new_val;
  return 0;
}

int cfgstr_set_float (ConfigString *cfgstr, const char *key, float value)
{
  char buf[512];
  snprintf(buf, sizeof buf, "%f", value);
  buf[sizeof buf - 1] = 0; /* make sure it's null terminated */

  return cfgstr_set(cfgstr, key, buf);
}

int cfgstr_set_int (ConfigString *cfgstr, const char *key, int value)
{
  char buf[32];
  snprintf(buf, sizeof buf, "%d", value);
  buf[sizeof buf - 1] = 0; /* make sure it's null terminated */

  return cfgstr_set(cfgstr, key, buf);
}

static char *strip_whitespace (char *buf)
{
	while (*buf && isspace(*buf))
		buf++;

	if (!*buf)
		return 0;

	char *end = buf + strlen(buf) - 1;
	while (end > buf && isspace(*end))
		end--;

	end[1] = 0;
	return buf;
}
