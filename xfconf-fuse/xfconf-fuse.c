
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#define FUSE_USE_VERSION 25
#include <fuse.h>

#include "xfconf/xfconf.h"

#define PTR_TO_UINT64(ptr)  ( (guint64)(ptr) )
#define UINT64_TO_PTR(i)    ( (gpointer)(i) )

static GHashTable *channel_cache = NULL;  /* (gchar *) -> (GHashTable *) */
static GHashTable *empty_dirs = NULL;

typedef struct
{
    gulong open_count;
    gchar *channel_name;
    gchar *property_name;
    gboolean is_branch;
} XfconfFuseHandle;

typedef struct
{
    gchar *property_base;
    void *buf;
    fuse_fill_dir_t filler;
    GHashTable *done;
} DirFillerData;


#if 0
static guint
count_slashes(const gchar *property_name)
{
    gchar *p = (gchar *)property_name;
    gint n = 2;

    if(!property_name || (property_name[0] == '/' && !property_name[1]))
        return 1;

    while((p = strchr(p+1, '/')))
        ++n;

    return n;
}
#endif

static gboolean
xfconf_fuse_decompose_path(const gchar *path,
                           gchar **channel_name,
                           gchar **property_name,
                           gboolean *is_branch)
{
    gchar *p, *q;

    if(G_UNLIKELY(path[0] == '/' && !path[1])) {
        *channel_name = NULL;
        *property_name = NULL;
        *is_branch = TRUE;
    } else {
        p = strstr(path+1, "/");
        if(p) {
            *channel_name = g_strndup(path+1, p-(path+1));
            q = p + strlen(p) - 5;
            if(!strcmp(q, ".prop")) {
                *property_name = g_strndup(p, q-p);
                *is_branch = FALSE;
            } else {
                *property_name = g_strdup(p);
                *is_branch = TRUE;
            }
        } else {
            *channel_name = g_strdup(path+1);
            *property_name = NULL;
            *is_branch = TRUE;
        }
    }

    return TRUE;
}

static int
xfconf_fuse_getattr_internal(const gchar *channel_name,
                             const gchar *property_name,
                             gboolean is_branch,
                             struct stat *statbuf)
{
    int ret = 0;
    XfconfChannel *channel = NULL;

    if(is_branch) {
        if(property_name) {
            channel = xfconf_channel_get(channel_name);
            GHashTable *properties = xfconf_channel_get_properties(channel,
                                                                   property_name);

            if(!properties)
                ret = -EIO;
            else if(g_hash_table_size(properties) == 0
                    || (g_hash_table_size(properties) == 1
                        && g_hash_table_lookup(properties, property_name)))
            {
                ret = -ENOENT;
            }

            g_hash_table_destroy(properties);
        } else if(channel_name) {
            gchar **channels = xfconf_list_channels();
            gint i;

            if(!channels)
                ret = -EIO;
            else {
                for(i = 0; channels[i]; ++i) {
                    if(!strcmp(channels[i], channel_name))
                        break;
                }

                if(!channels[i])
                    ret = -ENOENT;

                g_strfreev(channels);
            }
        } else {
            /* root element */
        }
    } else {
        /* both channel_name and property_name must be valid here */
        channel = xfconf_channel_get(channel_name);
        if(!xfconf_channel_has_property(channel, property_name))
            ret = -ENOENT;
    }

    if(ret == 0) {
        statbuf->st_uid = getuid();
        statbuf->st_gid = getgid();
        statbuf->st_mode = S_IRUSR;
        if(is_branch) {
            statbuf->st_mode |= S_IFDIR;
            statbuf->st_nlink = 2;
        } else {
            statbuf->st_mode |= S_IFREG;
            statbuf->st_nlink = 1;
        }

        if(channel_name) {
            if(!channel)
                channel = xfconf_channel_get(channel_name);
            if(!xfconf_channel_is_property_locked(channel, property_name
                                                           ? property_name
                                                           : "/"))
            {
                statbuf->st_mode |= S_IWUSR;
            }
        } else {
            /* root dir */
            statbuf->st_mode |= S_IWUSR;
        }
    }

    return ret;
}

static int
xfconf_fuse_getattr(const char *path,
                    struct stat *statbuf)
{
    int ret = 0;
    gchar *channel_name = NULL, *property_name = NULL;
    gboolean is_branch = FALSE;

    if(!xfconf_fuse_decompose_path(path, &channel_name, &property_name, &is_branch))
        return -ENOENT;

    ret = xfconf_fuse_getattr_internal(channel_name, property_name,
                                       is_branch, statbuf);

    g_free(channel_name);
    g_free(property_name);

    return ret;
}

static int
xfconf_fuse_opendir(const char *path,
                    struct fuse_file_info *finfo)
{
    return 0;
}

static void
fill_dir_from_hashtable(gpointer key,
                        gpointer value,
                        gpointer user_data)
{
    DirFillerData *dfdata = user_data;
    gchar *start, *end, buf[PATH_MAX];

    start = (gchar *)key + strlen(dfdata->property_base);
    if(start[0] == '/')
        ++start;

    if((end = strchr(start+1, '/'))) {
        g_strlcpy(buf, start, end-start+1 > sizeof(buf) ? sizeof(buf) : end-start+1);
        if(!g_hash_table_lookup(dfdata->done, buf)) {
            dfdata->filler(dfdata->buf, buf, NULL, 0);
            g_hash_table_insert(dfdata->done, g_strdup(buf), (gpointer)1);
        }
    } else {
        g_strlcpy(buf, start, sizeof(buf));
        g_strlcat(buf, ".prop", sizeof(buf));
        dfdata->filler(dfdata->buf, buf, NULL, 0);
    }
}

static int
xfconf_fuse_readdir(const char *path,
                    void *buf,
                    fuse_fill_dir_t filler,
                    off_t ofs,
                    struct fuse_file_info *finfo)
{
    gint ret = 0;
    gchar *channel_name = NULL, *property_name = NULL;
    gboolean is_branch = FALSE;

    if(!xfconf_fuse_decompose_path(path, &channel_name, &property_name, &is_branch))
        return -ENOENT;

    if(!is_branch) {
        g_free(channel_name);
        g_free(property_name);
        return -ENOTDIR;
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    if(!channel_name) {
        gchar **channels = xfconf_list_channels();

        if(!channels)
            ret = -EIO;
        else {
            gint i;

            for(i = 0; channels[i]; ++i)
                filler(buf, channels[i], NULL, 0);
            g_strfreev(channels);
        }
    } else {
        XfconfChannel *channel = xfconf_channel_get(channel_name);
        GHashTable *properties = xfconf_channel_get_properties(channel,
                                                               property_name);
        if(!properties)
            ret = -EIO;
        else {
            DirFillerData dfdata;
            
            dfdata.property_base = property_name ? property_name : "/";
            dfdata.buf = buf;
            dfdata.filler = filler;
            dfdata.done = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                (GDestroyNotify)g_free,
                                                NULL);

            g_hash_table_foreach(properties, fill_dir_from_hashtable, &dfdata);

            g_hash_table_destroy(dfdata.done);
            g_hash_table_destroy(properties);
        }
    }

    g_free(channel_name);
    g_free(property_name);

    return ret;
}

static int
xfconf_fuse_open(const char *path,
                 struct fuse_file_info *finfo)
{
    gint ret;
    struct stat statbuf;
    gchar *channel_name = NULL, *property_name = NULL;
    gboolean is_branch = FALSE;
    XfconfFuseHandle *xfh;

    if(finfo->fh) {
        /* FIXME: need to validate new permissions */
        xfh = UINT64_TO_PTR(finfo->fh);
        xfh->open_count++;
        return 0;
    }

    if(finfo->flags & O_APPEND)
        return -EACCES;

    if(!xfconf_fuse_decompose_path(path, &channel_name, &property_name, &is_branch))
        return -ENOENT;

    if(is_branch && (finfo->flags & (O_RDWR|O_WRONLY))) {
        ret = -EISDIR;
        goto out_err;
    }

    ret = xfconf_fuse_getattr_internal(channel_name, property_name,
                                       is_branch, &statbuf);
    if(ret == -ENOENT) {
        if(!(finfo->flags & (O_RDWR|O_WRONLY)))
            goto out_err;
    } else if(ret < 0)
        goto out_err;

    if(!(statbuf.st_mode & S_IWUSR) && (finfo->flags & (O_WRONLY|O_RDWR))) {
        ret = -EACCES;
        goto out_err;
    }

    xfh = g_slice_new0(XfconfFuseHandle);
    xfh->open_count = 1;
    xfh->channel_name = channel_name;
    xfh->property_name = property_name;
    xfh->is_branch = is_branch;
    finfo->fh = PTR_TO_UINT64(xfh);

    return 0;

out_err:

    if(xfh)
        g_free(xfh);
    g_free(channel_name);
    g_free(property_name);

    return ret;
}

static int
xfconf_fuse_read(const char *path,
                 char *buf,
                 size_t size,
                 off_t offset,
                 struct fuse_file_info *finfo)
{
    XfconfFuseHandle *xfh = UINT64_TO_PTR(finfo->fh);
    XfconfChannel *channel;
    GValue val = { 0, };
    const gchar *type_str;

    if(G_UNLIKELY(!xfh))
        return -EINVAL;

    channel = xfconf_channel_get(xfh->channel_name);
    if(!xfconf_channel_get_property(channel, xfh->property_name, &val))
        return -EIO;

    type_str = _xfconf_string_from_gtype(G_VALUE_TYPE(&val));
    if(!type_str)
        type_str = "empty";

    if(size < strlen(type_str) + 1 + G_VALUE_HOLDS_STRING(



}

static int
xfconf_fuse_release(const char *path,
                    struct fuse_file_info *finfo)
{
    XfconfFuseHandle *xfh = UINT64_TO_PTR(finfo->fh);

    if(G_UNLIKELY(!xfh))
        return -EINVAL;

    if(!--xfh->open_count) {
        g_free(xfh->channel_name);
        g_free(xfh->property_name);
        g_slice_free(XfconfFuseHandle, xfh);
        finfo->fh = 0;
    }

    return 0;
}


static struct fuse_operations xfconf_oper = {
    .getattr = xfconf_fuse_getattr,
    .opendir = xfconf_fuse_opendir,
    .readdir = xfconf_fuse_readdir,
    .open = xfconf_fuse_open,
    .read = xfconf_fuse_read,
//    .write = xfconf_fuse_write,
    .release = xfconf_fuse_release,
//    .unlink = xfconf_fuse_unlink,
//    .rmdir = xfconf_fuse_rmdir,
//    .mkdir = xfconf_fuse_mkdir,
};

int
main(int argc,
     char **argv)
{
    int ret;
    GError *error = NULL;

    if(!xfconf_init(&error)) {
        g_printerr("Failed to initialize xfconf: %s\n", error->message);
        g_error_free(error);
        return EXIT_FAILURE;
    }

    channel_cache = g_hash_table_new_full(g_str_hash, g_str_equal,
                                          (GDestroyNotify)g_free,
                                          (GDestroyNotify)g_hash_table_destroy);
    empty_dirs = g_hash_table_new_full(g_str_hash, g_str_equal,
                                       (GDestroyNotify)g_free, NULL);

    ret = fuse_main(argc, argv, &xfconf_oper);

    g_hash_table_destroy(channel_cache);
    channel_cache = NULL;
    g_hash_table_destroy(empty_dirs);
    empty_dirs = NULL;

    return ret;
}
