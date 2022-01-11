/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License ONLY.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __XFCONF_LIFECYCLE_MANAGER_H__
#define __XFCONF_LIFECYCLE_MANAGER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#if ! GLIB_CHECK_VERSION (2, 68, 0)
#define G_DBUS_METHOD_INVOCATION_HANDLED TRUE
#define G_DBUS_METHOD_INVOCATION_UNHANDLED FALSE
#endif

#define XFCONF_TYPE_LIFECYCLE_MANAGER xfconf_lifecycle_manager_get_type ()
G_DECLARE_FINAL_TYPE (XfconfLifecycleManager, xfconf_lifecycle_manager, XFCONF, LIFECYCLE_MANAGER, GObject)

typedef struct _XfconfLifecycleManagerPrivate XfconfLifecycleManagerPrivate;

struct _XfconfLifecycleManager
{
    GObject parent;
    XfconfLifecycleManagerPrivate *priv;
};

XfconfLifecycleManager* xfconf_lifecycle_manager_new (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

void xfconf_lifecycle_manager_start (XfconfLifecycleManager *manager);

gboolean xfconf_lifecycle_manager_keep_alive (XfconfLifecycleManager *manager);

gboolean xfconf_lifecycle_manager_increment_use_count (XfconfLifecycleManager *manager);

gboolean xfconf_lifecycle_manager_decrement_use_count (XfconfLifecycleManager *manager);

G_END_DECLS

#endif /* !__XFCONF_LIFECYCLE_MANAGER_H__ */
