/* hb-dvd.c
 *
 * Copyright (C) 2008-2025 John Stebbins <stebbins@stebbins>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "hb-dvd.h"

#ifdef _WIN32
#include <windows.h>
#endif

#if 0
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#ifndef PACKED
#define PACKED              __attribute__((packed))
#endif

struct volume_descriptor {
    struct descriptor_tag {
        guint16 id;
        guint16 version;
        guint8  checksum;
        guint8  reserved;
        guint16 serial;
        guint16 crc;
        guint16 crc_len;
        guint32 location;
    } PACKED tag;
    union {
        struct anchor_descriptor {
            guint32 length;
            guint32 location;
        } PACKED anchor;
        struct primary_descriptor {
            guint32 seq_num;
            guint32 desc_num;
            struct dstring {
                guint8  clen;
                guint8  c[31];
            } PACKED ident;
        } PACKED primary;
    } PACKED type;
} PACKED;

struct volume_structure_descriptor {
    guint8      type;
    guint8      id[5];
    guint8      version;
} PACKED;

#define VOLUME_ID_LABEL_SIZE        64
typedef struct
{
    gint fd;
    gchar label[VOLUME_ID_LABEL_SIZE+1];
} udf_info_t;

enum endian {
    LE = 0,
    BE = 1
};

#ifdef __BYTE_ORDER
#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#define le16_to_cpu(x) (x)
#define le32_to_cpu(x) (x)
#define le64_to_cpu(x) (x)
#define be16_to_cpu(x) bswap_16(x)
#define be32_to_cpu(x) bswap_32(x)
#define cpu_to_le16(x) (x)
#define cpu_to_le32(x) (x)
#define cpu_to_be32(x) bswap_32(x)
#elif (__BYTE_ORDER == __BIG_ENDIAN)
#define le16_to_cpu(x) bswap_16(x)
#define le32_to_cpu(x) bswap_32(x)
#define le64_to_cpu(x) bswap_64(x)
#define be16_to_cpu(x) (x)
#define be32_to_cpu(x) (x)
#define cpu_to_le16(x) bswap_16(x)
#define cpu_to_le32(x) bswap_32(x)
#define cpu_to_be32(x) (x)
#endif
#endif /* __BYTE_ORDER */

#define UDF_VSD_OFFSET          0x8000

static guint8*
get_buffer(int fd, guint64 off, gsize len)
{
    gint buf_len;

    if (lseek(fd, off, SEEK_SET) < 0)
    {
        return NULL;
    }
    guint8 *buffer = g_malloc(len);
    buf_len = read(fd, buffer, len);
    if (buf_len < 0)
    {
        g_free(buffer);
        return NULL;
    }
    return buffer;
}

static gint
set_unicode16(guint8 *str, gsize len, const guint8 *buf, gint endianness, gsize count)
{
    gint ii, jj;
    guint16 c;

    jj = 0;
    for (ii = 0; ii + 2 <= count; ii += 2) {
        if (endianness == LE)
            c = (buf[ii+1] << 8) | buf[ii];
        else
            c = (buf[ii] << 8) | buf[ii+1];
        if (c == 0) {
            str[jj] = '\0';
            break;
        } else if (c < 0x80) {
            if (jj+1 >= len)
                break;
            str[jj++] = (guint8) c;
        } else if (c < 0x800) {
            if (jj+2 >= len)
                break;
            str[jj++] = (guint8) (0xc0 | (c >> 6));
            str[jj++] = (guint8) (0x80 | (c & 0x3f));
        } else {
            if (jj+3 >= len)
                break;
            str[jj++] = (guint8) (0xe0 | (c >> 12));
            str[jj++] = (guint8) (0x80 | ((c >> 6) & 0x3f));
            str[jj++] = (guint8) (0x80 | (c & 0x3f));
        }
    }
    str[jj] = '\0';
    return jj;
}

static void
set_label_string(guint8 *str, const guint8 *buf, gsize count)
{
    gint ii;

    memcpy(str, buf, count);
    str[count] = 0;

    /* remove trailing whitespace */
    ii = strlen((gchar*)str);
    while (ii--)
    {
        if (!g_ascii_isspace(str[ii]))
            break;
    }
    str[ii+1] = 0;
}

static gint
probe_udf(udf_info_t *id)
{
    struct volume_descriptor *vd;
    struct volume_structure_descriptor *vsd;
    guint bs;
    guint b;
    guint type;
    guint count;
    guint loc;
    guint clen;
    guint64 off = 0;

    vsd = (struct volume_structure_descriptor *) get_buffer(id->fd, off + UDF_VSD_OFFSET, 0x200);
    if (vsd == NULL)
        return -1;

    if (memcmp(vsd->id, "NSR02", 5) == 0)
        goto blocksize;
    if (memcmp(vsd->id, "NSR03", 5) == 0)
        goto blocksize;
    if (memcmp(vsd->id, "BEA01", 5) == 0)
        goto blocksize;
    if (memcmp(vsd->id, "BOOT2", 5) == 0)
        goto blocksize;
    if (memcmp(vsd->id, "CD001", 5) == 0)
        goto blocksize;
    if (memcmp(vsd->id, "CDW02", 5) == 0)
        goto blocksize;
    if (memcmp(vsd->id, "TEA03", 5) == 0)
        goto blocksize;
    return -1;

blocksize:
    /* search the next VSD to get the logical block size of the volume */
    for (bs = 0x800; bs < 0x8000; bs += 0x800) {
        vsd = (struct volume_structure_descriptor *) get_buffer(id->fd, off + UDF_VSD_OFFSET + bs, 0x800);
        if (vsd == NULL)
            return -1;
        if (vsd->id[0] != '\0')
            goto nsr;
    }
    return -1;

nsr:
    /* search the list of VSDs for a NSR descriptor */
    for (b = 0; b < 64; b++) {
        vsd = (struct volume_structure_descriptor *) get_buffer(id->fd, off + UDF_VSD_OFFSET + (b * bs), 0x800);
        if (vsd == NULL)
            return -1;

        if (vsd->id[0] == '\0')
            return -1;
        if (memcmp(vsd->id, "NSR02", 5) == 0)
            goto anchor;
        if (memcmp(vsd->id, "NSR03", 5) == 0)
            goto anchor;
    }
    return -1;

anchor:
    /* read anchor volume descriptor */
    vd = (struct volume_descriptor *) get_buffer(id->fd, off + (256 * bs), 0x200);
    if (vd == NULL)
        return -1;

    type = le16_to_cpu(vd->tag.id);
    if (type != 2) /* TAG_ID_AVDP */
        goto found;

    /* get descriptor list address and block count */
    count = le32_to_cpu(vd->type.anchor.length) / bs;
    loc = le32_to_cpu(vd->type.anchor.location);

    /* pick the primary descriptor from the list */
    for (b = 0; b < count; b++) {
        vd = (struct volume_descriptor *) get_buffer(id->fd, off + ((loc + b) * bs), 0x200);
        if (vd == NULL)
            return -1;

        type = le16_to_cpu(vd->tag.id);

        /* check validity */
        if (type == 0)
            goto found;
        if (le32_to_cpu(vd->tag.location) != loc + b)
            goto found;

        if (type == 1) /* TAG_ID_PVD */
            goto pvd;
    }
    goto found;

pvd:
    clen = vd->type.primary.ident.clen;
    if (clen == 8)
        set_label_string((guint8*)id->label, vd->type.primary.ident.c, 31);
    else if (clen == 16)
        set_unicode16((guint8*)id->label, sizeof(id->label), vd->type.primary.ident.c, BE, 31);

found:
    return 0;
}

gchar*
ghb_dvd_volname(const gchar *device)
{
    udf_info_t id;
    gchar *buffer = NULL;

    id.fd = open(device, O_RDONLY);
    if (id.fd < 0) {
        return NULL;
    }
    if (probe_udf (&id) == 0)
    {
        buffer = g_strdup(id.label);
    }
    return buffer;
}
#endif

gchar*
ghb_resolve_symlink(const gchar *name)
{
    gchar *file;
    GFileInfo *info;
    GFile *gfile;

    gfile = g_file_new_for_path(name);
    info = g_file_query_info(gfile,
        G_FILE_ATTRIBUTE_STANDARD_NAME ","
        G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET ","
        G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK,
        G_FILE_QUERY_INFO_NONE, NULL, NULL);
    while ((info != NULL) && g_file_info_get_is_symlink(info))
    {
        GFile *parent;
        const gchar *target;

        parent = g_file_get_parent(gfile);
        g_object_unref(gfile);
        target = g_file_info_get_symlink_target(info);
        gfile = g_file_resolve_relative_path(parent, target);
        g_object_unref(parent);

        g_object_unref(info);
        info = g_file_query_info(gfile,
            G_FILE_ATTRIBUTE_STANDARD_NAME ","
            G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET ","
            G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK,
            G_FILE_QUERY_INFO_NONE, NULL, NULL);
    }
    if (info != NULL)
    {
        file = g_file_get_path(gfile);
        g_object_unref(info);
    }
    else
    {
        file = g_strdup(name);
    }
    g_object_unref(gfile);
    return file;
}

void
ghb_dvd_set_current(const gchar *name, signal_user_data_t *ud)
{
#if !defined(_WIN32)
    GFile *gfile;
    GFileInfo *info;
    gchar *resolved = ghb_resolve_symlink(name);

    if (ud->current_dvd_device != NULL)
    {
        g_free(ud->current_dvd_device);
        ud->current_dvd_device = NULL;
    }
    gfile = g_file_new_for_path(resolved);
    info = g_file_query_info(gfile,
        G_FILE_ATTRIBUTE_STANDARD_TYPE,
        G_FILE_QUERY_INFO_NONE, NULL, NULL);
    if (info != NULL)
    {
        if (g_file_info_get_file_type(info) == G_FILE_TYPE_SPECIAL)
        {
            // I could go through the trouble to scan the connected drives and
            // verify that this device is connected and is a DVD.  But I don't
            // think its necessary.
            ud->current_dvd_device = resolved;
        }
        else
        {
            g_free(resolved);
        }
        g_object_unref(info);
    }
    else
    {
        g_free(resolved);
    }
    g_object_unref(gfile);
#else
    gchar drive[4];
    guint dtype;

    if (ud->current_dvd_device != NULL)
    {
        g_free(ud->current_dvd_device);
        ud->current_dvd_device = NULL;
    }
    g_strlcpy(drive, name, 4);
    dtype = GetDriveType(drive);
    if (dtype == DRIVE_CDROM)
    {
        ud->current_dvd_device = g_strdup(name);
    }
#endif
}
