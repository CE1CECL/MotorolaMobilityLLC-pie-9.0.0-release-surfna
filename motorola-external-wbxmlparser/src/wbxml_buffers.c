/*
 * WBXML Lib, the WBXML Library.
 * Copyright (C) 2002-2003  Aymerick J�hanne
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License (version 2.1) as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * LGPL v2.1: http://www.gnu.org/licenses/lgpl.txt
 *
 * Author Contact: libwbxml@jehanne.org
 * WBXML Lib home: http://libwbxml.jehanne.org
 */

/*
 * Copyright (C) 2003 Motorola Inc.
 *
 *
 * Date       Author      Comments
 * -------------------------------------
 * 10/15/2003 Motorola    Add more strict data type
 */

/**
 * @file wbxml_buffers.c
 * @ingroup wbxml_buffers
 *
 * @brief Generic Buffers
 *
 * @author Aymerick J�hanne <libwbxml@jehanne.org>
 * @date 02/03/12
 *
 * @note Original idea: Kannel Project (http://www.kannel.org/)
 */
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "wbxml.h"


/* Memory management define */
#define WBXML_BUFFER_SPLIT_BLOCK 20

/**
 * The Generic Buffer type
 */



static WB_BOOL grow_buff(WBXMLBuffer *buffer, WB_ULONG size);
static WB_BOOL insert_data(WBXMLBuffer *buffer, WB_ULONG pos, const WB_UTINY *data, WB_ULONG len);



/**********************************
 *    Public functions
 */


WBXML_DECLARE(WBXMLBuffer *) wbxml_buffer_create_real(const WB_UTINY *data, WB_ULONG len, WB_ULONG malloc_block)
{
    WBXMLBuffer *buffer = NULL;

    buffer = (WBXMLBuffer *) wbxml_malloc(sizeof(WBXMLBuffer));
    if (buffer == NULL)
        return NULL;

    buffer->malloc_block = malloc_block;

    if ((len <= 0) || (data == NULL)) {
        buffer->malloced = 0;
        buffer->len = 0;
        buffer->data = NULL;
    }
    else {
        if (len + 1 > malloc_block + 1)
            buffer->malloced = len + 1 + malloc_block;
        else
            buffer->malloced = malloc_block + 1;

        buffer->data = (WB_UTINY *) wbxml_malloc(buffer->malloced * sizeof(WB_UTINY));
        if (buffer->data == NULL) {
            wbxml_free(buffer);
            return NULL;
        }

        buffer->len = len;
        memcpy(buffer->data, data, len);
        buffer->data[len] = '\0';
    }

    return buffer;
}


WBXML_DECLARE(void) wbxml_buffer_destroy(WBXMLBuffer *buffer)
{
    if (buffer != NULL) {
        wbxml_free(buffer->data);
        wbxml_free(buffer);
    }
}


WBXML_DECLARE_NONSTD(void) wbxml_buffer_destroy_item(void *buff)
{
    wbxml_buffer_destroy((WBXMLBuffer *) buff);
}


WBXML_DECLARE(WBXMLBuffer *) wbxml_buffer_duplicate(WBXMLBuffer *buff)
{
    WBXMLBuffer *result = NULL;

    if (buff == NULL)
        return NULL;

    result = wbxml_buffer_create_real(wbxml_buffer_get_cstr(buff),
                                      wbxml_buffer_len(buff),
                                      wbxml_buffer_len(buff));

    return result;
}


WBXML_DECLARE(WB_ULONG) wbxml_buffer_len(WBXMLBuffer *buffer)
{
    if (buffer == NULL)
        return 0;

    return buffer->len;
}


WBXML_DECLARE(WB_BOOL) wbxml_buffer_get_char(WBXMLBuffer *buffer, WB_ULONG pos, WB_UTINY *result)
{
    if ((buffer == NULL) || (pos >= buffer->len))
        return FALSE;

    *result = buffer->data[pos];
    return TRUE;
}


WBXML_DECLARE(void) wbxml_buffer_set_char(WBXMLBuffer *buffer, WB_ULONG pos, WB_UTINY ch)
{
    if (pos < buffer->len)
        buffer->data[pos] = ch;
}


WBXML_DECLARE(WB_UTINY *) wbxml_buffer_get_cstr(WBXMLBuffer *buffer)
{
    if ((buffer == NULL) || (buffer->len == 0))
        return (WB_UTINY *)"";

    return buffer->data;
}


WBXML_DECLARE(WB_BOOL) wbxml_buffer_insert(WBXMLBuffer *to, WBXMLBuffer *buffer, WB_ULONG pos)
{
    if ((to != NULL) && (buffer != NULL))
        return insert_data(to, pos, buffer->data, buffer->len);

    return FALSE;
}


WBXML_DECLARE(WB_BOOL) wbxml_buffer_insert_cstr(WBXMLBuffer *to, WB_UTINY *str, WB_ULONG pos)
{
    if ((to != NULL) && (str != NULL))
        return insert_data(to, pos, str, WBXML_STRLEN(str));

    return FALSE;
}


WBXML_DECLARE(WB_BOOL) wbxml_buffer_append(WBXMLBuffer *dest, WBXMLBuffer *buff)
{
    if (dest == NULL)
        return FALSE;

    if (buff == NULL)
        return TRUE;

    return wbxml_buffer_append_data(dest, wbxml_buffer_get_cstr(buff), wbxml_buffer_len(buff));
}


WBXML_DECLARE(WB_BOOL) wbxml_buffer_append_data(WBXMLBuffer *buffer, const WB_UTINY *data, WB_ULONG len)
{
    if (buffer == NULL)
        return FALSE;

    if ((data == NULL) || (len == 0))
        return TRUE;

    return insert_data(buffer, buffer->len, data, len);
}

WBXML_DECLARE(WB_BOOL) wbxml_buffer_append_cstr(WBXMLBuffer *buffer, const WB_UTINY *data)
{
    if (data == NULL)
        return TRUE;

    return wbxml_buffer_append_data(buffer, data, WBXML_STRLEN(data));
}


WBXML_DECLARE(WB_BOOL) wbxml_buffer_append_char(WBXMLBuffer *buffer, WB_UTINY ch)
{
    unsigned char c = ch;

    if (buffer == NULL)
        return FALSE;

    return insert_data(buffer, buffer->len, &c, 1);
}


WBXML_DECLARE(WB_BOOL) wbxml_buffer_append_mb_uint_32(WBXMLBuffer *buffer, WB_ULONG value)
{
    /* A uintvar is defined to be up to 32 bits large
     * so it will fit in 5 octets (to handle continuation bits)
    */
    WB_UTINY octets[5];
    WB_LONG i, start;

    /* Handle last byte separately; it has no continuation bit,
     * and must be encoded even if value is 0.
     */
    octets[4] = (WB_UTINY) (value & 0x7f);
    value >>= 7;

    for (i = 3; value > 0 && i >= 0; i--) {
        octets[i] = (WB_UTINY) (0x80 | (value & 0x7f));
        value >>= 7;
    }
    start = i + 1;

    return wbxml_buffer_append_data(buffer, octets + start, 5 - start);
}


WBXML_DECLARE(void) wbxml_buffer_delete(WBXMLBuffer *buffer, WB_ULONG pos, WB_ULONG len)
{
    if (pos > buffer->len)
        pos = buffer->len;

    if (pos + len > buffer->len)
        len = buffer->len - pos;

    if (len > 0) {
        memmove(buffer->data + pos, buffer->data + pos + len,
                buffer->len - pos - len);

        buffer->len -= len;
        buffer->data[buffer->len] = '\0';
    }
}


WBXML_DECLARE(void) wbxml_buffer_shrink_blanks(WBXMLBuffer *buffer)
{
    WB_ULONG i = 0, j = 0, end = wbxml_buffer_len(buffer);
    WB_UTINY ch;

    for (i = 0; i < end; i++)
    {
        if (wbxml_buffer_get_char(buffer, i, &ch) && isspace(ch))
        {
            /* Replace space by a whitespace */
            if (ch != ' ')
                wbxml_buffer_set_char(buffer, i, ' ');

            /* Remove all following spaces */
            j = i = i + 1;
            while (wbxml_buffer_get_char(buffer, j, &ch) && isspace(ch))
                j++;

            if (j - i > 1)
                wbxml_buffer_delete(buffer, i, j - i);
        }
    }
}


WBXML_DECLARE(void) wbxml_buffer_strip_blanks(WBXMLBuffer *buffer)
{
    WB_LONG start = 0, end, len = 0;
    WB_UTINY ch;

    /* Remove whitespaces at beginning of buffer... */
    while (wbxml_buffer_get_char(buffer, start, &ch) &&
           isspace(ch) &&
	       start <= (WB_LONG) wbxml_buffer_len(buffer))
    {
        start ++;
    }

    if (start > 0)
        wbxml_buffer_delete(buffer, 0, start);

    /* ... and at the end */
    if ((len = wbxml_buffer_len(buffer)) > 0) {
        end = len = len - 1;
        while (wbxml_buffer_get_char(buffer, end, &ch) &&
            isspace(ch) &&
            end >= 0)
        {
            end--;
        }
        wbxml_buffer_delete(buffer, end + 1, len - end);
    }
}


WBXML_DECLARE(WB_LONG) wbxml_buffer_compare(WBXMLBuffer *buff1, WBXMLBuffer *buff2)
{
    WB_LONG ret, len;

    if (buff1->len < buff2->len)
        len = buff1->len;
    else
        len = buff2->len;

    if (len == 0)
    {
	    if (buff1->len == 0 && buff2->len > 0)
	        return -1;
	    if (buff1->len > 0 && buff2->len == 0)
	        return 1;
        return 0;
    }

    if ((ret = memcmp(buff1->data, buff2->data, len)) == 0)
    {
        if (buff1->len < buff2->len)
            ret = -1;
        else {
            if (buff1->len > buff2->len)
                ret = 1;
        }
    }

    return ret;
}


WBXML_DECLARE(WBXMLList *) wbxml_buffer_split_words_real(WBXMLBuffer *buff)
{
    WB_UTINY *p = NULL;
    WBXMLList *list = NULL;
    WBXMLBuffer *word;
    WB_ULONG i, start, end;

    if ((list = wbxml_list_create()) == NULL)
        return NULL;

    p = buff->data;
    i = 0;
    while (TRUE)
    {
        while (i < buff->len && isspace(*p)) {
            ++p;
            ++i;
        }
        start = i;

        while (i < buff->len && !isspace(*p)) {
            ++p;
            ++i;
        }
        end = i;

        if (start == end)
            break;

        if((word = wbxml_buffer_create(buff->data + start, end - start, WBXML_BUFFER_SPLIT_BLOCK)) == NULL) {
            wbxml_list_destroy(list, wbxml_buffer_destroy_item);
            return NULL;
        }

        wbxml_list_append(list, word);
    }

    return list;
}


WBXML_DECLARE(WB_BOOL) wbxml_buffer_search_char(WBXMLBuffer *to, WB_UTINY ch, WB_ULONG pos, WB_ULONG *result)
{
    WB_UTINY *p = NULL;

    if (to == NULL)
        return FALSE;

    if (pos >= to->len)
        return FALSE;

    if ((p = (WB_UTINY *) memchr(to->data + pos, ch, to->len - pos)) == NULL)
        return FALSE;

    if (result != NULL)
        *result = p - to->data;

    return TRUE;
}


WBXML_DECLARE(WB_BOOL) wbxml_buffer_search(WBXMLBuffer *to, WBXMLBuffer *search, WB_ULONG pos, WB_ULONG *result)
{
    WB_UTINY first = 0;

    if ((to == NULL) || (search == NULL))
        return FALSE;

    if (result != NULL)
        *result = 0;

    /* Always "find" an empty string */
    if (search->len == 0)
        return TRUE;

    /* Check if 'search' is greater than 'to' */
    if (search->len > to->len)
        return FALSE;

    /* We are searching for one char */
    if (search->len == 1)
        return wbxml_buffer_search_char(to, search->data[0], pos, result);

    /* For each occurrence of search's first character in to, then check if the rest of needle follows.
     * Stop if there are no more occurrences, or if the rest of 'search' can't possibly fit in 'to'. */
    first = search->data[0];
    while ((wbxml_buffer_search_char(to, first, pos, &pos)) &&
           (to->len - pos >= search->len))
    {
        if (memcmp(to->data + pos, search->data, search->len) == 0) {
            if (result != NULL)
                *result = pos;
            return TRUE;
        }
        pos++;
    }

    return FALSE;
}


WBXML_DECLARE(WB_BOOL) wbxml_buffer_search_cstr(WBXMLBuffer *to, WB_UTINY *search, WB_ULONG pos, WB_ULONG *result)
{
    WB_UTINY first = 0;

    if ((to == NULL) || (search == NULL))
        return FALSE;

    if (result != NULL)
        *result = 0;

    /* Always "find" an empty string */
    if (WBXML_STRLEN(search) == 0)
        return TRUE;

    /* Check if 'search' is greater than 'to' */
    if (WBXML_STRLEN(search) > to->len)
        return FALSE;

    /* We are searching for one char */
    if (WBXML_STRLEN(search) == 1)
        return wbxml_buffer_search_char(to, search[0], pos, result);

    /* For each occurrence of search's first character in to, then check if the rest of needle follows.
     * Stop if there are no more occurrences, or if the rest of 'search' can't possibly fit in 'to'. */
    first = search[0];
    while ((wbxml_buffer_search_char(to, first, pos, &pos)) &&
           (to->len - pos >= WBXML_STRLEN(search)))
    {
        if (memcmp(to->data + pos, search, WBXML_STRLEN(search)) == 0) {
            if (result != NULL)
                *result = pos;
            return TRUE;
        }
        pos++;
    }

    return FALSE;
}


WBXML_DECLARE(WB_BOOL) wbxml_buffer_contains_only_whitespaces(WBXMLBuffer *buffer)
{
    WB_ULONG i;

    if (buffer == NULL)
        return FALSE;

    for (i=0; i<buffer->len; i++) {
        if (!isspace(*(buffer->data + i)))
            return FALSE;
    }

    return TRUE;
}


WBXML_DECLARE(void) wbxml_buffer_hex_to_binary(WBXMLBuffer *buffer)
{
    WB_UTINY *p = NULL;
    WB_ULONG i = 0, len = 0;

    if (buffer == NULL)
        return;

    p = buffer->data;
    len = wbxml_buffer_len(buffer);

    /* Convert ascii data to binary values */
    for (i = 0; i < len; i++, p++) {
        if (*p >= '0' && *p <= '9')
            *p -= '0';
        else if (*p >= 'a' && *p <= 'f')
            *p = *p - 'a' + 10;
        else if (*p >= 'A' && *p <= 'F')
            *p = *p - 'A' + 10;
        else {
            /* Bad Bad ! There should be only digits in the buffer ! */
            *p = 0;
        }
    }

    /* De-hexing will compress data by factor of 2 */
    len = buffer->len / 2;

    for (i = 0; i < len; i++)
        buffer->data[i] = buffer->data[i * 2] * 16 | buffer->data[i * 2 + 1];

    buffer->len = len;
    buffer->data[len] = '\0';
}


WBXML_DECLARE(WB_BOOL) wbxml_buffer_binary_to_hex(WBXMLBuffer *buffer, WB_BOOL uppercase)
{
    WB_UTINY *hexits = NULL;
    WB_LONG i = 0;

    if (buffer == NULL)
        return FALSE;

    if (wbxml_buffer_len(buffer) == 0)
        return TRUE;

    hexits = (WB_UTINY *)(uppercase ? "0123456789ABCDEF" : "0123456789abcdef");

    /* Grows the Buffer size by 2 */
    grow_buff(buffer, buffer->len * 2);

    /* In-place modification must be done back-to-front to avoid
     * overwriting the data while we read it.  Even the order of
     * the two assignments is important, to get i == 0 right.
     */
    for (i = buffer->len - 1; i >= 0; i--) {
        buffer->data[i * 2 + 1] = hexits[buffer->data[i] % 16];
        buffer->data[i * 2] = hexits[(buffer->data[i] / 16) & 0xf];
    }

    buffer->len = buffer->len * 2;
    buffer->data[buffer->len] = '\0';

    return TRUE;
}


WBXML_DECLARE(void) wbxml_buffer_remove_trailing_zeros(WBXMLBuffer **buffer)
{
    WB_UTINY ch = 0;

    if ((buffer == NULL) || (*buffer == NULL))
        return;

    while ((*buffer)->len > 0) {
        if (wbxml_buffer_get_char(*buffer, wbxml_buffer_len(*buffer) - 1, &ch) && (ch == '\0'))
            wbxml_buffer_delete(*buffer, wbxml_buffer_len(*buffer) - 1, 1);
        else
            return;
    }
}


/**********************************
 *    Private functions
 */

/**
 * @brief Add space for at least 'size' octets
 * @param buffer The buffer
 * @param size The size to add
 * @return TRUE is space successfully reserved, FALSE is size was negative, buffer was NULL or if not enough memory
 */
static WB_BOOL grow_buff(WBXMLBuffer *buffer, WB_ULONG size)
{
    if (buffer == NULL)
        return FALSE;

    /* Make room for the invisible terminating NUL */
    size++;

    if ((buffer->len + size) > buffer->malloced) {
        if ((buffer->malloced + buffer->malloc_block) < (buffer->len + size))
            buffer->malloced = buffer->len + size + buffer->malloc_block;
        else
            buffer->malloced = buffer->malloced + buffer->malloc_block;

        buffer->data = (WB_UTINY *)wbxml_realloc(buffer->data, buffer->malloced);
        if (buffer->data == NULL)
            return FALSE;
    }

    return TRUE;
}


/**
 * @brief Insert data into a Generic Buffer
 * @param buffer The Generic Buffer
 * @param pos Position in Generic Buffer where to insert data
 * @param data Data to insert
 * @param len Data length
 * @return TRUE is data inserted, FALSE if not
 */
static WB_BOOL insert_data(WBXMLBuffer *buffer, WB_ULONG pos, const WB_UTINY *data, WB_ULONG len)
{
    if ((buffer == NULL) || (len == 0) || (pos > buffer->len))
        return FALSE;

    if (!grow_buff(buffer, len))
        return FALSE;

    if (buffer->len > pos) {
        /* Only if neccessary */
        memmove(buffer->data + pos + len, buffer->data + pos, buffer->len - pos);
    }

    memcpy(buffer->data + pos, data, len);
    buffer->len += len;
    buffer->data[buffer->len] = '\0';

    return TRUE;
}
