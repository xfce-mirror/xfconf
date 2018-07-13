/*
 * Copyright (C) 2018 - Ali Abdallah <ali@xfce.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include <glib.h>

#include "xfconf/xfconf.h"

#include "xfconf-gsettings-backend.h"


struct _XfconfGsettingsBackend
{
  GSettingsBackend __parent__;

  XfconfChannel    *channel;
};

G_DEFINE_TYPE (XfconfGsettingsBackend, xfconf_gsettings_backend, G_TYPE_SETTINGS_BACKEND);

static GVariant *
xfconf_gsettings_backend_read (GSettingsBackend   *backend,
                               const gchar        *key,
                               const GVariantType *expected_type,
                               gboolean            default_value)
{
  return NULL;
}

static void
xfconf_gsettings_backend_reset (GSettingsBackend   *backend,
                                const gchar        *key,
                                gpointer            origin_tag)
{
}

static gboolean
xfconf_gsettings_backend_get_writable (GSettingsBackend *backend,
                                       const gchar      *key)
{
  return FALSE;
}

static gboolean
xfconf_gsettings_backend_write_tree (GSettingsBackend *backend,
                                     GTree            *tree,
                                     gpointer          origin_tag)
{
  return FALSE;
}

static gboolean
xfconf_gsettings_backend_write (GSettingsBackend *backend,
                                const gchar      *key,
                                GVariant         *value,
                                gpointer          origin_tag)
{

  return FALSE;
}

static void
xfconf_gsettings_backend_finalize (XfconfGsettingsBackend *self)
{
}

static void
xfconf_gsettings_backend_init (XfconfGsettingsBackend *self)
{
  const gchar *prg_name;

  prg_name = g_get_prgname();

  self->channel = xfconf_channel_new (prg_name);
}

static void
xfconf_gsettings_backend_class_init (XfconfGsettingsBackendClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GSettingsBackendClass *gsettings_class = G_SETTINGS_BACKEND_CLASS (klass);

  gsettings_class->read = xfconf_gsettings_backend_read;
  gsettings_class->reset = xfconf_gsettings_backend_reset;
  gsettings_class->get_writable = xfconf_gsettings_backend_get_writable;
  gsettings_class->write_tree = xfconf_gsettings_backend_write_tree;
  gsettings_class->write = xfconf_gsettings_backend_write;

  object_class->finalize = (void (*) (GObject *object)) xfconf_gsettings_backend_finalize;
}

XfconfGsettingsBackend* xfconf_gsettings_backend_new (void)
{
  XfconfGsettingsBackend *xfconf_gsettings = g_object_new (XFCONF_GSETTINGS_BACKEND_TYPE, NULL);
  return xfconf_gsettings;
}
