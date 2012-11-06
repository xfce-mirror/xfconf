/*
 *  xfconf
 *
 *  Copyright (c) 2009 Brian Tarricone <brian@tarricone.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; version 2
 *  of the License ONLY.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "xfconf-cache.h"
#include "xfconf-channel.h"
#include "xfconf-errors.h"
#include "xfconf-dbus-bindings.h"
#include "common/xfconf-gvaluefuncs.h"
#include "xfconf-private.h"
#include "common/xfconf-marshal.h"
#include "common/xfconf-common-private.h"
#if 0
#include "xfconf-types.h"
#include "xfconf.h"
#include "xfconf-alias.h"
#endif

#if 0
#define DEFAULT_MAX_ENTRIES  -1  /* no limit */
#define DEFAULT_MAX_AGE      (60*60)  /* 1 hour */
#endif

#define ALIGN_VAL(val, align)  ( ((val) + ((align) -1)) & ~((align) - 1) )



#if GLIB_CHECK_VERSION (2, 32, 0)
#define xfconf_cache_mutex_lock(cache)   g_mutex_lock (&(cache)->cache_lock)
#define xfconf_cache_mutex_unlock(cache) g_mutex_unlock (&(cache)->cache_lock)
#else
#define xfconf_cache_mutex_lock(cache)   g_mutex_lock ((cache)->cache_lock)
#define xfconf_cache_mutex_unlock(cache) g_mutex_unlock ((cache)->cache_lock)
#endif



/**************** XfconfCacheItem ****************/


typedef struct
{
#if 0
    GTimeVal last_used;
#endif
    GValue *value;
} XfconfCacheItem;

static XfconfCacheItem *
xfconf_cache_item_new(const GValue *value,
                      gboolean steal)
{
    XfconfCacheItem *item;

    g_return_val_if_fail(value, NULL);

    item = g_slice_new0(XfconfCacheItem);
#if 0
    g_get_current_time(&item->last_used);
#endif

    if(G_LIKELY(steal)) {
        item->value = (GValue *) value;
    } else {
        item->value = g_new0(GValue, 1);
        g_value_init(item->value, G_VALUE_TYPE(value));
        g_value_copy(value, item->value);
    }

    return item;
}

static gboolean
xfconf_cache_item_update(XfconfCacheItem *item,
                         const GValue *value)
{
    if(value && _xfconf_gvalue_is_equal(item->value, value))
        return FALSE;

#if 0
    g_get_current_time(&item->last_used);
#endif

    if(value) {
        g_value_unset(item->value);
        g_value_init(item->value, G_VALUE_TYPE(value));
        g_value_copy(value, item->value);

        return TRUE;
    }

    return FALSE;
}

static void
xfconf_cache_item_free(XfconfCacheItem *item)
{
    g_return_if_fail(item);

    g_value_unset(item->value);
    g_free(item->value);
    g_slice_free(XfconfCacheItem, item);
}


/******************* XfconfCacheOldItem *******************/


typedef struct
{
    gchar *property;
    DBusGProxyCall *call;
    XfconfCacheItem *item;
} XfconfCacheOldItem;

static XfconfCacheOldItem *
xfconf_cache_old_item_new(const gchar *property)
{
    XfconfCacheOldItem *old_item;

    g_return_val_if_fail(property, NULL);

    old_item = g_slice_new0(XfconfCacheOldItem);
    old_item->property = g_strdup(property);

    return old_item;
}

static void
xfconf_cache_old_item_free(XfconfCacheOldItem *old_item)
{
    g_return_if_fail(old_item);

    /* debug check to make sure the call is properly handled before
     * freeing the item. it should either been cancelled or we wait for
     * it to finish */
    g_return_if_fail(!old_item->call);

    g_free(old_item->property);

    if(old_item->item)
        xfconf_cache_item_free(old_item->item);

    g_slice_free(XfconfCacheOldItem, old_item);
}

static gboolean
xfconf_cache_old_item_end_call(gpointer key,
                               gpointer value,
                               gpointer user_data)
{
    const gchar *channel_name = user_data;
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    GError *error = NULL;
    XfconfCacheOldItem *old_item = value;

    g_return_val_if_fail(old_item->call, TRUE);

    if(!dbus_g_proxy_end_call(proxy, old_item->call, &error, G_TYPE_INVALID)) {
       g_warning("Failed to set property \"%s::%s\": %s",
                  channel_name, old_item->property, error->message);
        g_error_free(error);
    }

    old_item->call = NULL;

    return TRUE;
}


/************************* XfconfCache ********************/


/**
 * XfconfCache:
 *
 * An opaque structure that holds state about a cache.
 **/
struct _XfconfCache
{
    GObject parent;

    gchar *channel_name;

#if 0
    gint max_entries;
    gint max_age;
#endif

    GTree *properties;

    GHashTable *pending_calls;
    GHashTable *old_properties;

#if GLIB_CHECK_VERSION (2, 32, 0)
    GMutex cache_lock;
#else
    GMutex *cache_lock;
#endif
};

typedef struct _XfconfCacheClass
{
    GObjectClass parent;

    void (*property_changed)(XfconfCache *cache,
                             const gchar *channel_name,
                             const gchar *property,
                             const GValue *value);
} XfconfCacheClass;

enum
{
    SIG_PROPERTY_CHANGED = 0,
    N_SIGS,
};

enum
{
    PROP0 = 0,
    PROP_CHANNEL_NAME,
#if 0
    PROP_MAX_ENTRIES,
    PROP_MAX_AGE,
#endif
};

static void xfconf_cache_set_g_property(GObject *object,
                                        guint property_id,
                                        const GValue *value,
                                        GParamSpec *pspec);
static void xfconf_cache_get_g_property(GObject *object,
                                        guint property_id,
                                        GValue *value,
                                        GParamSpec *pspec);
static void xfconf_cache_finalize(GObject *obj);

static void xfconf_cache_property_changed(DBusGProxy *proxy,
                                          const gchar *cache_name,
                                          const gchar *property,
                                          const GValue *value,
                                          gpointer user_data);
static void xfconf_cache_property_removed(DBusGProxy *proxy,
                                          const gchar *cache_name,
                                          const gchar *property,
                                          gpointer user_data);


static guint signals[N_SIGS] = { 0, };


G_DEFINE_TYPE(XfconfCache, xfconf_cache, G_TYPE_OBJECT)


static void
xfconf_cache_class_init(XfconfCacheClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;

    object_class->set_property = xfconf_cache_set_g_property;
    object_class->get_property = xfconf_cache_get_g_property;
    object_class->finalize = xfconf_cache_finalize;

    signals[SIG_PROPERTY_CHANGED] = g_signal_new(I_("property-changed"),
                                                 XFCONF_TYPE_CACHE,
                                                 G_SIGNAL_RUN_LAST,
                                                 G_STRUCT_OFFSET(XfconfCacheClass,
                                                                 property_changed),
                                                 NULL,
                                                 NULL,
                                                 _xfconf_marshal_VOID__STRING_STRING_BOXED,
                                                 G_TYPE_NONE,
                                                 3, G_TYPE_STRING,
                                                 G_TYPE_STRING,
                                                 G_TYPE_VALUE);

    g_object_class_install_property(object_class, PROP_CHANNEL_NAME,
                                    g_param_spec_string("channel-name",
                                                        "Channel Name",
                                                        "The name of the channel managed by the cache",
                                                        NULL,
                                                        G_PARAM_READWRITE
                                                        | G_PARAM_CONSTRUCT_ONLY
                                                        | G_PARAM_STATIC_NAME
                                                        | G_PARAM_STATIC_NICK
                                                        | G_PARAM_STATIC_BLURB));
#if 0
    g_object_class_install_property(object_class, PROP_MAX_ENTRIES,
                                    g_param_spec_int("max-entries",
                                                     "Maximum entries",
                                                     "Maximum number of cache entries to hold at once",
                                                     -1, G_MAXINT,
                                                     DEFAULT_MAX_ENTRIES,
                                                     G_PARAM_READWRITE
                                                     | G_PARAM_CONSTRUCT
                                                     | G_PARAM_STATIC_NAME
                                                     | G_PARAM_STATIC_NICK
                                                     | G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class, PROP_MAX_AGE,
                                    g_param_spec_int("max-age",
                                                     "Maximum age",
                                                     "Maximum age (in seconds) before an entry gets evicted from the cache",
                                                     0, G_MAXINT,
                                                     DEFAULT_MAX_AGE,
                                                     G_PARAM_READWRITE
                                                     | G_PARAM_CONSTRUCT
                                                     | G_PARAM_STATIC_NAME
                                                     | G_PARAM_STATIC_NICK
                                                     | G_PARAM_STATIC_BLURB));
#endif
}

static void
xfconf_cache_init(XfconfCache *cache)
{
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();

    dbus_g_proxy_connect_signal(proxy, "PropertyChanged",
                                G_CALLBACK(xfconf_cache_property_changed),
                                cache, NULL);
    dbus_g_proxy_connect_signal(proxy, "PropertyRemoved",
                                G_CALLBACK(xfconf_cache_property_removed),
                                cache, NULL);

    cache->properties = g_tree_new_full((GCompareDataFunc)strcmp, NULL,
                                        (GDestroyNotify)g_free,
                                        (GDestroyNotify)xfconf_cache_item_free);

    cache->pending_calls = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                                 NULL,
                                                 (GDestroyNotify)xfconf_cache_old_item_free);
    cache->old_properties = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                  NULL, NULL);

#if GLIB_CHECK_VERSION (2, 32, 0)
    g_mutex_init (&cache->cache_lock);
#else
    cache->cache_lock = g_mutex_new ();
#endif
}

static void
xfconf_cache_set_g_property(GObject *object,
                            guint property_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
    XfconfCache *cache = XFCONF_CACHE(object);

    switch(property_id) {
        case PROP_CHANNEL_NAME:
            g_free(cache->channel_name);
            cache->channel_name = g_value_dup_string(value);
            break;
#if 0
        case PROP_MAX_ENTRIES:
            xfconf_cache_set_max_entries(cache, g_value_get_int(value));
            break;

        case PROP_MAX_AGE:
            xfconf_cache_set_max_age(cache, g_value_get_int(value));
            break;
#endif
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
xfconf_cache_get_g_property(GObject *object,
                            guint property_id,
                            GValue *value,
                            GParamSpec *pspec)
{
    XfconfCache *cache = XFCONF_CACHE(object);

    switch(property_id) {
        case PROP_CHANNEL_NAME:
            g_value_set_string(value, cache->channel_name);
            break;
#if 0
        case PROP_MAX_ENTRIES:
            g_value_set_int(value, cache->max_entries);
            break;

        case PROP_MAX_AGE:
            g_value_set_int(value, cache->max_age);
            break;
#endif
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
xfconf_cache_finalize(GObject *obj)
{
    XfconfCache *cache = XFCONF_CACHE(obj);
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    GHashTable *pending_calls;

    dbus_g_proxy_disconnect_signal(proxy, "PropertyChanged",
                                   G_CALLBACK(xfconf_cache_property_changed),
                                   cache);

    dbus_g_proxy_disconnect_signal(proxy, "PropertyRemoved",
                                   G_CALLBACK(xfconf_cache_property_removed),
                                   cache);

    /* finish pending calls (without emitting signals, therefore we set
     * the hash table in the cache to %NULL) */
    pending_calls = cache->pending_calls;
    cache->pending_calls = NULL;
    g_hash_table_foreach_remove(pending_calls, xfconf_cache_old_item_end_call,
                                cache->channel_name);
    g_hash_table_unref(pending_calls);

    g_free(cache->channel_name);

    g_tree_destroy(cache->properties);
    g_hash_table_destroy(cache->old_properties);

#if !GLIB_CHECK_VERSION (2, 32, 0)
    g_mutex_free (cache->cache_lock);
#endif

    G_OBJECT_CLASS(xfconf_cache_parent_class)->finalize(obj);
}



static void
xfconf_cache_property_changed(DBusGProxy *proxy,
                              const gchar *channel_name,
                              const gchar *property,
                              const GValue *value,
                              gpointer user_data)
{
    XfconfCache *cache = XFCONF_CACHE(user_data);
    XfconfCacheItem *item;
    gboolean changed = TRUE;

    if(strcmp(channel_name, cache->channel_name))
        return;

    /* if a call was cancelled, we still receive a property-changed from
     * that value, in that case, abort the emission of the signal. we can
     * detect this because the new reply is not processed yet and thus
     * there is still an old_prop in the hash table */
    if(g_hash_table_lookup(cache->old_properties, property))
        return;

    item = g_tree_lookup(cache->properties, property);
    if(item)
        changed = xfconf_cache_item_update(item, value);
    else {
        item = xfconf_cache_item_new(value, FALSE);
        g_tree_insert(cache->properties, g_strdup(property), item);
    }

    if(changed) {
        g_signal_emit(G_OBJECT(cache), signals[SIG_PROPERTY_CHANGED], 0,
                      cache->channel_name, property, value);
    }
}

static void
xfconf_cache_property_removed(DBusGProxy *proxy,
                              const gchar *channel_name,
                              const gchar *property,
                              gpointer user_data)
{
    XfconfCache *cache = XFCONF_CACHE(user_data);
    GValue value = { 0, };

    if(strcmp(channel_name, cache->channel_name))
        return;

    g_tree_remove(cache->properties, property);

    g_signal_emit(G_OBJECT(cache), signals[SIG_PROPERTY_CHANGED], 0,
                  cache->channel_name, property, &value);
}



static void
xfconf_cache_set_property_reply_handler(DBusGProxy *proxy,
                                        DBusGProxyCall *call,
                                        gpointer user_data)
{
    XfconfCache *cache = user_data;
    XfconfCacheOldItem *old_item = NULL; 
    XfconfCacheItem *item;
    GError *error = NULL;

    if(!cache->pending_calls)
        return;

    xfconf_cache_mutex_lock(cache);

    old_item = g_hash_table_lookup(cache->pending_calls, call);
    if(G_UNLIKELY(!old_item)) {
#ifndef NDEBUG
        g_debug("Couldn't find old cache item based on pending call (libxfconf bug?)");
#endif
        goto out;
    }

    g_hash_table_remove(cache->old_properties, old_item->property);
    /* don't destroy old_item yet */
    g_hash_table_steal(cache->pending_calls, old_item->call);

    item = g_tree_lookup(cache->properties, old_item->property);
    if(G_UNLIKELY(!item)) {
#ifndef NDEBUG
        g_debug("Couldn't find current cache item based on pending call (libxfconf bug?)");
#endif
        goto out;
    }

    if(!dbus_g_proxy_end_call(proxy, call, &error, G_TYPE_INVALID)) {
        /* failed to set the value.  reset it to the old value and send
         * a prop changed signal to the channel */
        GValue empty_val = { 0, };

        g_warning("Failed to set property \"%s::%s\": %s",
                  cache->channel_name, old_item->property, error->message);
        g_error_free(error);

        if(old_item->item)
            xfconf_cache_item_update(item, old_item->item->value);
        else {
            g_tree_remove(cache->properties, old_item->property);
            item = NULL;
        }

        /* we need to drop the lock when running the signal handlers */
        xfconf_cache_mutex_unlock(cache);
        g_signal_emit(G_OBJECT(cache), signals[SIG_PROPERTY_CHANGED],
                      g_quark_from_string(old_item->property),
                      cache->channel_name, old_item->property,
                      item ? item->value : &empty_val);
        xfconf_cache_mutex_lock(cache);
    }

    /* we handled the call, so set it to %NULL */
    old_item->call = NULL;

    if(old_item)
        xfconf_cache_old_item_free(old_item);
out:
    xfconf_cache_mutex_unlock(cache);
}



#if 0
static void
xfconf_cache_reset_property_reply_handler(DBusGProxy *proxy,
                                          DBusGProxyCall *call,
                                          gpointer user_data)
{
    XfconfCache *cache = user_data;
    XfconfCacheOldItem *old_item;
    GError *error = NULL;

    xfconf_cache_mutex_lock(cache);

    old_item = g_hash_table_lookup(cache->pending_calls, call);
    if(G_UNLIKELY(!old_item)) {
        g_critical("Couldn't find old cache item based on pending call (libxfconf bug?)");
        goto out;
    }

    if(!dbus_g_proxy_end_call(proxy, call, &error, G_TYPE_INVALID)) {
        g_warning("Failed to reset property \"%s::%s\": %s",
                  cache->channel_name, old_item->property, error->message);
        g_error_free(error);
    }

out:
    if(old_item)
        g_hash_table_remove(cache->pending_calls, old_item->call);

    xfconf_cache_mutex_unlock(cache);
}
#endif



XfconfCache *
xfconf_cache_new(const gchar *channel_name)
{
    return g_object_new(XFCONF_TYPE_CACHE,
                        "channel-name", channel_name,
                        NULL);
}

static gboolean
xfconf_cache_prefetch_ht(gpointer key,
                         gpointer value,
                         gpointer user_data)
{
    XfconfCache *cache = XFCONF_CACHE(user_data);
    XfconfCacheItem *item;

    item = xfconf_cache_item_new(value, TRUE);
    g_tree_insert(cache->properties, key, item);

    return TRUE;
}

gboolean
xfconf_cache_prefetch(XfconfCache *cache,
                      const gchar *property_base,
                      GError **error)
{
    gboolean ret = FALSE;
    GHashTable *props = NULL;
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    GError *tmp_error = NULL;

    g_return_val_if_fail(g_tree_nnodes(cache->properties) == 0, FALSE);

    xfconf_cache_mutex_lock(cache);

    if(xfconf_client_get_all_properties(proxy, cache->channel_name,
                                        property_base ? property_base : "/",
                                        &props, &tmp_error))
    {
        g_hash_table_foreach_steal(props, xfconf_cache_prefetch_ht, cache);
        g_hash_table_destroy(props);
        /* TODO: honor max entries */
        ret = TRUE;
    } else
        g_propagate_error(error, tmp_error);

    xfconf_cache_mutex_unlock(cache);

    return ret;
}

static gboolean
xfconf_cache_lookup_locked(XfconfCache *cache,
                           const gchar *property,
                           GValue *value,
                           GError **error)
{
    XfconfCacheItem *item = NULL;

    item = g_tree_lookup(cache->properties, property);
    if(!item) {
        DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
        GValue tmpval = { 0, };
        GError *tmp_error = NULL;

        /* blocking, ugh */
        if(xfconf_client_get_property(proxy, cache->channel_name,
                                      property, &tmpval, &tmp_error))
        {
            item = xfconf_cache_item_new(&tmpval, FALSE);
            g_tree_insert(cache->properties, g_strdup(property), item);
            g_value_unset(&tmpval);
            /* TODO: check tree for evictions */
        } else
            g_propagate_error(error, tmp_error);
    }

    if(item) {
        if(value) {
            if(!G_VALUE_TYPE(value))
                g_value_init(value, G_VALUE_TYPE(item->value));

            if(G_VALUE_TYPE(value) == G_VALUE_TYPE(item->value))
                g_value_copy(item->value, value);
            else {
                if(!g_value_transform(item->value, value))
                    item = NULL;
            }
        }
#if 0
        if(item)
            xfconf_cache_item_update(item, NULL);
#endif
    }

    return !!item;
}

gboolean
xfconf_cache_lookup(XfconfCache *cache,
                    const gchar *property,
                    GValue *value,
                    GError **error)
{
    gboolean ret;

    g_return_val_if_fail(XFCONF_IS_CACHE(cache) && property
                         && (!error || !*error), FALSE);

    xfconf_cache_mutex_lock(cache);
    ret = xfconf_cache_lookup_locked(cache, property, value, error);
    xfconf_cache_mutex_unlock(cache);

    return ret;
}

gboolean
xfconf_cache_set(XfconfCache *cache,
                 const gchar *property,
                 const GValue *value,
                 GError **error)
{
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    XfconfCacheItem *item = NULL;
    XfconfCacheOldItem *old_item = NULL;

    xfconf_cache_mutex_lock(cache);

    item = g_tree_lookup(cache->properties, property);
    if(!item) {
        /* this is really quite the opposite of what we want here,
         * but i can't think of a better way yet. */
        GValue tmp_val = { 0, };
        GError *tmp_error = NULL;

        if(!xfconf_cache_lookup_locked(cache, property, &tmp_val, &tmp_error)) {
            /* this is just another example of dbus-glib's brain-deadedness.
             * instead of remapping the remote error back into the local
             * domain and code, it uses DBUS_GERROR as the domain,
             * DBUS_GERROR_REMOTE_EXCEPTION as the code, and then "hides"
             * the full string ("org.xfce.Xfconf.Error.Whatever") in
             * GError::message after a NUL byte.  so stupid. */
            const gchar *dbus_error_name = NULL;

            if(G_LIKELY(tmp_error->domain == DBUS_GERROR
                        && tmp_error->code == DBUS_GERROR_REMOTE_EXCEPTION))
            {
                dbus_error_name = dbus_g_error_get_name(tmp_error);
            }

            if(g_strcmp0(dbus_error_name, "org.xfce.Xfconf.Error.PropertyNotFound") != 0
               && g_strcmp0(dbus_error_name, "org.xfce.Xfconf.Error.ChannelNotFound") != 0)
            {
                /* this is bad... */
                g_propagate_error(error, tmp_error);
                xfconf_cache_mutex_unlock(cache);
                return FALSE;
            }

            /* prop just doesn't exist; continue */
            g_error_free(tmp_error);
        } else {
            g_value_unset(&tmp_val);
            item = g_tree_lookup(cache->properties, property);
        }
    }

    if(item) {
        /* if the value isn't changing, there's no reason to continue */
        if(_xfconf_gvalue_is_equal(item->value, value)) {
            xfconf_cache_mutex_unlock(cache);
            return TRUE;
        }
    }

    old_item = g_hash_table_lookup(cache->old_properties, property);
    if(old_item) {
        /* if we have an old item, it means that a previous set
         * call hasn't returned yet.  let's cancel that call and
         * throw away the current not-yet-committed value of
         * the property.
         * we also steal the old_item from the pending_calls table
         * so there are no pending item left. */
        if(old_item->call) {
            dbus_g_proxy_cancel_call(proxy, old_item->call);
            g_hash_table_steal(cache->pending_calls, old_item->call);
            old_item->call = NULL;
        }
    } else {
        old_item = xfconf_cache_old_item_new(property);
        if(item)
            old_item->item = xfconf_cache_item_new(item->value, FALSE);
        g_hash_table_insert(cache->old_properties, old_item->property, old_item);
    }

    /* can't use the generated dbus-glib binding here cuz we won't
     * get the pending call pointer in the callback */
    old_item->call = dbus_g_proxy_begin_call(proxy, "SetProperty",
                                             xfconf_cache_set_property_reply_handler,
                                             cache, NULL,
                                             G_TYPE_STRING, cache->channel_name,
                                             G_TYPE_STRING, property,
                                             G_TYPE_VALUE, value,
                                             G_TYPE_INVALID);
    g_hash_table_insert(cache->pending_calls, old_item->call, old_item);

    if(item)
        xfconf_cache_item_update(item, value);
    else {
        item = xfconf_cache_item_new(value, FALSE);
        g_tree_insert(cache->properties, g_strdup(property), item);
    }

    xfconf_cache_mutex_unlock(cache);

    g_signal_emit(G_OBJECT(cache), signals[SIG_PROPERTY_CHANGED], 0,
                  cache->channel_name, property, value);

    return TRUE;
}

typedef struct
{
    gchar *property_base;
    gsize property_base_len;
    GSList *matches;
} XfconfCacheRecurseData;

static gboolean
xfconf_cache_collect_properties_recursive(gpointer key,
                                          gpointer value,
                                          gpointer user_data)
{
    gchar *property_name = key;
    XfconfCacheRecurseData *rdata = user_data;

    if(!g_ascii_strncasecmp(rdata->property_base, property_name, rdata->property_base_len))
        rdata->matches = g_slist_prepend(rdata->matches, property_name);

    return FALSE;
}

gboolean
xfconf_cache_reset(XfconfCache *cache,
                   const gchar *property_base,
                   gboolean recursive,
                   GError **error)
{
    gboolean ret = FALSE;
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
#if 0
    XfconfCacheOldItem *old_item = NULL;
#endif

    xfconf_cache_mutex_lock(cache);

#if 0
    /* it's not really feasible here to look up all the old/new values
     * here, so we're just gonna rely on the normal signals from the
     * daemon to notify us of changes */

    /* can't use the generated dbus-glib binding here cuz we won't
     * get the pending call pointer in the callback */
    old_item = xfconf_cache_old_item_new(property_base);
    old_item->call = dbus_g_proxy_begin_call(proxy, "ResetProperty",
                                             xfconf_cache_reset_property_reply_handler,
                                             cache, NULL,
                                             G_TYPE_STRING, cache->channel_name,
                                             G_TYPE_STRING, property_base,
                                             G_TYPE_BOOLEAN, recursive,
                                             G_TYPE_INVALID);
    if(old_item->call) {
        g_hash_table_insert(cache->pending_calls, old_item->call, old_item);
        ret = TRUE;
    } else {
        if(error) {
            g_set_error(error, DBUS_GERROR, DBUS_GERROR_FAILED,
                        _("Failed to make ResetProperty DBus call"));
        }
    }
#else
    /* unfortunately, doing the above asynchronously makes
     * xfconf_channel_has_property() break, because we have no idea at
     * this point if a reset is going to remove the property or reset
     * it to a default.  so, we have to do this sync.  sad. */

    ret = xfconf_client_reset_property(proxy, cache->channel_name,
                                       property_base, recursive, error);

    if(ret) {
        /* here we just evict the entry from the cache if we have one.
         * unfortunately i think it's the best we can do here.  this is
         * pretty slow because we have to traverse the entire tree if
         * recursive==TRUE. */

        g_tree_remove(cache->properties, property_base);

        if(recursive) {
            XfconfCacheRecurseData rdata;
            GSList *l;

            rdata.property_base = g_strdup_printf("%s/", property_base);
            rdata.property_base_len = strlen(rdata.property_base);
            rdata.matches = NULL;

            g_tree_foreach(cache->properties,
                           xfconf_cache_collect_properties_recursive,
                           &rdata);

            for(l = rdata.matches; l; l = l->next)
                g_tree_remove(cache->properties, l->data);

            g_free(rdata.property_base);
            g_slist_free(rdata.matches);
        }
    }
#endif

    xfconf_cache_mutex_unlock(cache);

    return ret;
}

#if 0
void
xfconf_cache_set_max_entries(XfconfCache *cache,
                             gint max_entries)
{
    xfconf_cache_mutex_lock(cache);
    cache->max_entries = max_entries;
    /* TODO: check tree for eviction */
    xfconf_cache_mutex_unlock(cache);
}

gint
xfconf_cache_get_max_entries(XfconfCache *cache)
{
    return cache->max_entries;
}

void
xfconf_cache_set_max_age(XfconfCache *cache,
                         gint max_age)
{
    xfconf_cache_mutex_lock(cache);
    cache->max_age = max_age;
    /* TODO: check tree for eviction */
    xfconf_cache_mutex_unlock(cache);
}

gint
xfconf_cache_get_max_age(XfconfCache *cache)
{
    return cache->max_age;
}
#endif
