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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 *
 */

#ifndef MOCK_BAMF_MOCK_APPLICATION
#define MOCK_BAMF_MOCK_APPLICATION

#include <libbamf/libbamf.h>

G_BEGIN_DECLS

#define BAMF_TYPE_MOCK_APPLICATION (bamf_mock_application_get_type ())

#define BAMF_MOCK_APPLICATION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
        BAMF_TYPE_MOCK_APPLICATION, BamfMockApplication))

#define BAMF_MOCK_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
        BAMF_TYPE_MOCK_APPLICATION, BamfMockApplicationClass))

#define BAMF_IS_MOCK_APPLICATION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
        BAMF_TYPE_MOCK_APPLICATION))

#define BAMF_IS_MOCK_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
        BAMF_TYPE_MOCK_APPLICATION))

#define BAMF_MOCK_APPLICATION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
        BAMF_TYPE_MOCK_APPLICATION, BamfMockApplicationClass))

typedef struct _BamfMockApplication        BamfMockApplication;
typedef struct _BamfMockApplicationClass   BamfMockApplicationClass;
typedef struct _BamfMockApplicationPrivate BamfMockApplicationPrivate;

struct _BamfMockApplication
{
  BamfApplication parent;

  BamfMockApplicationPrivate *priv;
};

struct _BamfMockApplicationClass
{
  BamfApplicationClass parent_class;
};

GType bamf_mock_application_get_type (void) G_GNUC_CONST;

BamfMockApplication * bamf_mock_application_new ();

void bamf_mock_application_set_active (BamfMockApplication * self, gboolean active);
void bamf_mock_application_set_running (BamfMockApplication * self, gboolean running);
void bamf_mock_application_set_urgent (BamfMockApplication * self, gboolean urgent);
void bamf_mock_application_set_name (BamfMockApplication * self, const gchar * name);
void bamf_mock_application_set_icon (BamfMockApplication * self, const gchar * icon);
void bamf_mock_application_set_children (BamfMockApplication * self, GList * children);

G_END_DECLS

#endif

