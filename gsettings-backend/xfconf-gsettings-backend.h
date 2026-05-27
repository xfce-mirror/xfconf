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
 *
 */

#ifndef __XFCONF_GSETTINGS_BACKEND_H__
#define __XFCONF_GSETTINGS_BACKEND_H__

#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

G_BEGIN_DECLS

#define XFCONF_TYPE_GSETTINGS_BACKEND (xfconf_gsettings_backend_get_type())
G_DECLARE_FINAL_TYPE(XfconfGsettingsBackend, xfconf_gsettings_backend, XFCONF, GSETTINGS_BACKEND, GSettingsBackend)

XfconfGsettingsBackend *xfconf_gsettings_backend_new(void);

G_END_DECLS

#endif /* __XFCONF_GSETTINGS_BACKEND_H__ */
