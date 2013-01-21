// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 *
 */

#ifndef MOCK_BAMF_MOCK_WINDOW
#define MOCK_BAMF_MOCK_WINDOW

#include <time.h>
#include <glib-object.h>
#include <libbamf/libbamf.h>

G_BEGIN_DECLS

#define BAMF_TYPE_MOCK_WINDOW (bamf_mock_window_get_type ())

#define BAMF_MOCK_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
        BAMF_TYPE_MOCK_WINDOW, BamfMockWindow))

#define BAMF_MOCK_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
        BAMF_TYPE_MOCK_WINDOW, BamfMockWindowClass))

#define BAMF_IS_MOCK_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
        BAMF_TYPE_MOCK_WINDOW))

#define BAMF_IS_MOCK_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
        BAMF_TYPE_MOCK_WINDOW))

#define BAMF_MOCK_WINDOW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
        BAMF_TYPE_MOCK_WINDOW, BamfMockWindowClass))

typedef struct _BamfMockWindow        BamfMockWindow;
typedef struct _BamfMockWindowClass   BamfMockWindowClass;
typedef struct _BamfMockWindowPrivate BamfMockWindowPrivate;

struct _BamfMockWindow
{
  BamfWindow parent;

  BamfMockWindowPrivate *priv;
};

struct _BamfMockWindowClass
{
  BamfWindowClass parent_class;
};

GType bamf_mock_window_get_type (void) G_GNUC_CONST;

BamfMockWindow * bamf_mock_window_new ();

void    bamf_mock_window_set_transient      (BamfMockWindow *self, BamfWindow* transient);
void    bamf_mock_window_set_window_type    (BamfMockWindow *self, BamfWindowType window_type);
void    bamf_mock_window_set_xid            (BamfMockWindow *self, guint32 xid);
void    bamf_mock_window_set_pid            (BamfMockWindow *self, guint32 pid);
void    bamf_mock_window_set_monitor        (BamfMockWindow *self, gint monitor);
void    bamf_mock_window_set_utf8_prop      (BamfMockWindow *self, const char* prop, const char* value);
void    bamf_mock_window_set_maximized      (BamfMockWindow *self, BamfWindowMaximizationType maximized);
void    bamf_mock_window_set_last_active    (BamfMockWindow *self, time_t last_active);

G_END_DECLS

#endif
