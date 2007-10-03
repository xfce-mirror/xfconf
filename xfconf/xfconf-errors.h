/*
 *  xfconf
 *
 *  Copyright (c) 2007 Brian Tarricone <bjt23@cornell.edu>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License ONLY.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __XFCONF_ERRORS_H__
#define __XFCONF_ERRORS_H__

#if !defined(LIBXFCONF_COMPILATION) && !defined(XFCONF_IN_XFCONF_H)
#error "Do not include xfconf-errors.h, as this file may change or disappear in the future.  Include <xfconf/xfconf.h> instead."
#endif

#include <glib-object.h>

#define XFCONF_TYPE_ERROR  (xfconf_error_get_type())
#define XFCONF_ERROR       (xfconf_get_error_quark())

G_BEGIN_DECLS

typedef enum
{
    XFCONF_ERROR_UNKNOWN = 0,
    XFCONF_ERROR_CHANNEL_NOT_FOUND,
    XFCONF_ERROR_PROPERTY_NOT_FOUND,
    XFCONF_ERROR_READ_FAILURE,
    XFCONF_ERROR_WRITE_FAILURE,
    XFCONF_ERROR_PERMISSION_DENIED,
    XFCONF_ERROR_INTERNAL_ERROR,
    XFCONF_ERROR_NO_BACKEND,
} XfconfError;

GType xfconf_error_get_type() G_GNUC_CONST;
GQuark xfconf_get_error_quark();

G_END_DECLS

#endif  /* __XFCONF_ERRORS_H__ */
