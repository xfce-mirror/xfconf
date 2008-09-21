/*
 *  xfconf
 *
 *  Copyright (c) 2007-2008 Brian Tarricone <bjt23@cornell.edu>
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

#ifndef __XFCONF_PROPTREE_H__
#define __XFCONF_PROPTREE_H__

#include <glib-object.h>

#define MAX_PROP_PATH    (4096)

G_BEGIN_DECLS

typedef struct
{
    gchar *name;
    GValue value;
    gboolean locked;
} XfconfProperty;


GNode *xfconf_proptree_add_property(GNode *proptree,
                                    const gchar *name,
                                    const GValue *value,
                                    gboolean locked);
XfconfProperty *xfconf_proptree_lookup(GNode *proptree,
                                       const gchar *name);
GNode *xfconf_proptree_lookup_node(GNode *proptree,
                                   const gchar *name);
gboolean xfconf_proptree_remove(GNode *proptree,
                                const gchar *name);
void xfconf_proptree_destroy(GNode *proptree);

void xfconf_property_free(XfconfProperty *property);

G_END_DECLS

#endif  /* __XFCONF_PROPTREE_H__ */
