/*
 * MMC Transaction Logging driver
 *
 * Copyright (C) 2011 Intrinsyc Software Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <linux/vmalloc.h>
#include <linux/semaphore.h>
#include <linux/mmc/mmc_log.h>
#include <asm/uaccess.h>


/*****************************************************************************
 * Configuration
 *****************************************************************************/

#define MMC_LOG_BUFFER_NUM_ENTRIES  5000000

/* Text Format: mmmmmmmmmm o AAAAAAAAAA NNNNNNNNNNn0 */
#define MMC_LOG_MAX_STR_LEN              36

#define MMC_LOG_PROC_NAME         "mmc_log"


/*****************************************************************************
 * Structs, etc
 *****************************************************************************/

typedef struct mmc_log_entry
{
    unsigned int   time_ms;
    unsigned int   mmc_op : 1;
    unsigned int   block_num : 31;
    unsigned short num_blocks;
} __attribute__ ((packed)) S_MMC_LOG_ENTRY, *PS_MMC_LOG_ENTRY;


/*****************************************************************************
 * Global Variables
 *****************************************************************************/

static PS_MMC_LOG_ENTRY g_ps_log_buffer;
static struct mutex     g_buffer_mutex;
static atomic_t         g_log_opened;

static int  g_read_index;
static int  g_write_index;
static int  g_num_unread_entries;
static bool g_log_overlapped;
static bool g_enable_logging = false;


/*****************************************************************************
 * External Logging Functions
 *****************************************************************************/

int mmc_log_transaction(u8 mmc_operation, u32 block_addr, u32 num_blocks)
{
    const unsigned long timestamp_jiffies = jiffies;
    int  num_written_bytes = 0;
    bool read_index_passed = false;


    if (!try_module_get(THIS_MODULE))
    {
        goto error_return;
    }

    mutex_lock(&g_buffer_mutex);

    if (!g_enable_logging)
    {
        goto error_mutex_unlock;
    }

    switch (mmc_operation)
    {
        case MMC_OP_WRITE:
        case MMC_OP_READ:
            /* Valid */
            break;

        default:
            /* Invalid */
            printk(KERN_ERR "MMC LOG:%s() - ERROR: Unsupported MMC Operation specified: %u.\n", __FUNCTION__, mmc_operation);
            goto error_mutex_unlock;
    }

    g_ps_log_buffer[g_write_index].time_ms = jiffies_to_msecs(timestamp_jiffies);
    g_ps_log_buffer[g_write_index].mmc_op  = (mmc_operation & 0x01);

    g_ps_log_buffer[g_write_index].block_num  = (block_addr & 0x7FFFFFFF);
    g_ps_log_buffer[g_write_index].num_blocks = (num_blocks & 0xFFFF);

    read_index_passed = (g_log_overlapped && (g_read_index == g_write_index));

    g_write_index++;

    if (MMC_LOG_BUFFER_NUM_ENTRIES == g_write_index)
    {
        g_write_index = 0;
        g_log_overlapped = true;
    }

    if (MMC_LOG_BUFFER_NUM_ENTRIES != g_num_unread_entries)
    {
        g_num_unread_entries++;
    }

    /* Advance the Read Index if the buffer is in Overlap Mode and
     * if we have written directly past the current Read Index.
     */
    if (read_index_passed)
    {
        g_read_index = g_write_index;
    }

    num_written_bytes = sizeof(S_MMC_LOG_ENTRY);

error_mutex_unlock:
    mutex_unlock(&g_buffer_mutex);
    module_put(THIS_MODULE);

error_return:
    return num_written_bytes;
}


/*****************************************************************************
 * File Operation Functions
 *****************************************************************************/

static int mmc_log_open(struct inode * inode, struct file * file)
{
    int retval = 0;


    if (!try_module_get(THIS_MODULE))
    {
        retval = -ENODEV;
        goto error_return;
    }

    if (0 == atomic_add_unless(&g_log_opened, 1, 1))
    {
        /* Buffer is already open */
        retval = -EBUSY;
        goto error_module_put;
    }

    goto error_return;

error_module_put:
    module_put(THIS_MODULE);

error_return:
    return retval;
}

static int mmc_log_release(struct inode * inode, struct file * file)
{
    atomic_dec(&g_log_opened);
    module_put(THIS_MODULE);

    return 0;
}

static ssize_t mmc_log_read(struct file *p_file, char __user *buf, size_t count, loff_t *ppos)
{
    char szLogEntry[MMC_LOG_MAX_STR_LEN];
    ssize_t num_read_chars = 0;
    unsigned int entry_length;


    mutex_lock(&g_buffer_mutex);

    while ((count > 0) && (g_num_unread_entries > 0))
    {
        snprintf(szLogEntry,
                 MMC_LOG_MAX_STR_LEN,
                 "%u %c %u %u\n",
                 g_ps_log_buffer[g_read_index].time_ms,
                 ((MMC_OP_WRITE == g_ps_log_buffer[g_read_index].mmc_op) ? 'w' : 'r'),
                 g_ps_log_buffer[g_read_index].block_num,
                 g_ps_log_buffer[g_read_index].num_blocks);

        entry_length = strlen(szLogEntry);

        if (entry_length > count)
        {
            /* Not enough space left in the buffer for this entry. */
            break;
        }

        if (0 != copy_to_user(buf + num_read_chars, szLogEntry, entry_length))
        {
            printk(KERN_ERR "MMC LOG:%s() - ERROR: Could not transfer log entry %d to the given buffer.\n", __FUNCTION__, g_read_index);
            goto error_return;
        }

        num_read_chars += entry_length;
        count -= entry_length;
        
        g_num_unread_entries--;

        g_read_index++;
        if (MMC_LOG_BUFFER_NUM_ENTRIES == g_read_index)
        {
            g_read_index = 0;
        }
    }

error_return:
    mutex_unlock(&g_buffer_mutex);
    return num_read_chars;
}

static const struct file_operations g_mmc_log_fops = {
	.read		= mmc_log_read,
	.open		= mmc_log_open,
	.release	= mmc_log_release,
};


/*****************************************************************************
 * Module initialization/deinitialization
 *****************************************************************************/

static int __init mmc_log_init(void)
{
    int retval = 0;


    if (NULL == proc_create(MMC_LOG_PROC_NAME, S_IRUGO, NULL, &g_mmc_log_fops))
    {
        printk(KERN_ERR "MMC LOG:%s() - ERROR: Could not create entry for mmc_log in /proc.\n", __FUNCTION__);
        retval = -ENOMEM;
        goto error_return;
    }

    mutex_init(&g_buffer_mutex);

    /* Allocate log buffer */

    g_ps_log_buffer = vmalloc(MMC_LOG_BUFFER_NUM_ENTRIES * sizeof(S_MMC_LOG_ENTRY));
    if (NULL == g_ps_log_buffer)
    {
        printk(KERN_ERR "MMC LOG:%s() - ERROR: Could not allocate %d bytes of memory (GFP_KERNEL) for the Log Buffer.\n", __FUNCTION__, MMC_LOG_BUFFER_NUM_ENTRIES * sizeof(S_MMC_LOG_ENTRY));
        retval = -ENOMEM;
        goto error_mutex;
    }

    g_write_index = 0;
    g_read_index  = 0;

    g_log_overlapped     = false;
    g_num_unread_entries = 0;
    g_enable_logging     = true;

    atomic_set(&g_log_opened, 0);

    printk(KERN_INFO "MMC LOG:%s() - MMC Logging ENABLED.\n", __FUNCTION__);

    goto error_return;

error_mutex:
    mutex_destroy(&g_buffer_mutex);
    remove_proc_entry(MMC_LOG_PROC_NAME, NULL);

error_return:
    return retval;
}

void mmc_log_cleanup(void)
{
    mutex_lock(&g_buffer_mutex);

    g_enable_logging = false;
    printk(KERN_INFO "MMC LOG:%s() - MMC Logging DISABLED.\n", __FUNCTION__);

    remove_proc_entry(MMC_LOG_PROC_NAME, NULL);

    vfree(g_ps_log_buffer);

    mutex_unlock(&g_buffer_mutex);

    mutex_destroy(&g_buffer_mutex);
}

module_init(mmc_log_init);
module_exit(mmc_log_cleanup);

