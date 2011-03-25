/* NOTE: THIS FILE WAS POSSIBLY AUTO-GENERATED! */

/*
 * Copyright (c) 2008 Brian Tarricone <bjt23@cornell.edu>
 *
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

#include "xfconfperl.h"


MODULE = Xfce4::Xfconf    PACKAGE = Xfce4::Xfconf    PREFIX = xfconf_
PROTOTYPES: ENABLE

BOOT:
    {
#include "register.xsh"
#include "boot.xsh"

        gperl_handle_logs_for("Xfconf");
    }

gboolean
xfconf_init(class=NULL)
    PREINIT:
        GError *error = NULL;
    CODE:
        RETVAL = xfconf_init(&error);
        if(!RETVAL)
            gperl_croak_gerror(NULL, error);

void
xfconf_shutdown(class=NULL)
    C_ARGS:
        /* void */

#void
#xfconf_named_struct_register(struct_name, n_members, member_types)
#        const gchar * struct_name
#        guint n_members
#        const GType * member_types
