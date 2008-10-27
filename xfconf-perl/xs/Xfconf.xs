/* NOTE: THIS FILE WAS POSSIBLY AUTO-GENERATED! */

/*
 * Copyright (c) 2008 Brian Tarricone <bjt23@cornell.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
