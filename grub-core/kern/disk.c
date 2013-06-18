/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/disk.h>
#include <grub/err.h>
#include <grub/mm.h>
#include <grub/types.h>
#include <grub/partition.h>
#include <grub/misc.h>
#include <grub/time.h>
#include <grub/file.h>
#include <grub/i18n.h>

#define	GRUB_CACHE_TIMEOUT	2

/* The last time the disk was used.  */
static grub_uint64_t grub_last_time = 0;


/* Disk cache.  */
struct grub_disk_cache
{
  enum grub_disk_dev_id dev_id;
  unsigned long disk_id;
  grub_disk_addr_t sector;
  char *data;
  int lock;
};

static struct grub_disk_cache grub_disk_cache_table[GRUB_DISK_CACHE_NUM];

void (*grub_disk_firmware_fini) (void);
int grub_disk_firmware_is_tainted;

#if DISK_CACHE_STATS
static unsigned long grub_disk_cache_hits;
static unsigned long grub_disk_cache_misses;

void
grub_disk_cache_get_performance (unsigned long *hits, unsigned long *misses)
{
  *hits = grub_disk_cache_hits;
  *misses = grub_disk_cache_misses;
}
#endif

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 获得对应磁盘设备的扇区在disk cache数组中的索引。
*
* @note 注释详细内容:
*
* 本函数实现获得对应磁盘设备的扇区在disk cache数组中的索引的功能。实际是使用参数dev_id，
* disk_id，以及sector按照哈希表方式做映射。
**/
static unsigned
grub_disk_cache_get_index (unsigned long dev_id, unsigned long disk_id,
			   grub_disk_addr_t sector)
{
  return ((dev_id * 524287UL + disk_id * 2606459UL
	   + ((unsigned) (sector >> GRUB_DISK_CACHE_BITS)))
	  % GRUB_DISK_CACHE_NUM);
}
/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 使得对应磁盘设备的扇区的缓存数据在disk cache中无效。
*
* @note 注释详细内容:
*
* 本函数实现使得对应磁盘设备的扇区的缓存数据在disk cache中无效的功能。通过调用函数
* grub_disk_cache_get_index()获得在disk cache数组的索引，进而获得对应的cache项，如果
* 该cache项与参数一致（命中），那么就释放该缓存项，并使得该项数据无效。
**/
static void
grub_disk_cache_invalidate (unsigned long dev_id, unsigned long disk_id,
			    grub_disk_addr_t sector)
{
  unsigned index;
  struct grub_disk_cache *cache;

  sector &= ~(GRUB_DISK_CACHE_SIZE - 1);
  index = grub_disk_cache_get_index (dev_id, disk_id, sector);
  cache = grub_disk_cache_table + index;

  if (cache->dev_id == dev_id && cache->disk_id == disk_id
      && cache->sector == sector && cache->data)
    {
      cache->lock = 1;
      grub_free (cache->data);
      cache->data = 0;
      cache->lock = 0;
    }
}

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 使得对应磁盘设备的扇区的缓存数据在disk cache中无效。
*
* @note 注释详细内容:
*
* 本函数实现使得磁盘设备的扇区的所有缓存数据在disk cache中无效的功能。对总共所有
* GRUB_DISK_CACHE_NUM项的cache项，就释放该缓存项，并使得该项数据无效。
**/
void
grub_disk_cache_invalidate_all (void)
{
  unsigned i;

  for (i = 0; i < GRUB_DISK_CACHE_NUM; i++)
    {
      struct grub_disk_cache *cache = grub_disk_cache_table + i;

      if (cache->data && ! cache->lock)
	{
	  grub_free (cache->data);
	  cache->data = 0;
	}
    }
}
/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 获得对应磁盘设备的扇区在disk cache中的有效缓存数据并锁定该cache项。
*
* @note 注释详细内容:
*
* 本函数实现获得对应磁盘设备的扇区在disk cache中的有效缓存数据的功能。通过调用函数
* grub_disk_cache_get_index()获得在disk cache数组的索引，进而获得对应的cache项，如果
* 该cache项与参数一致（命中），那么就返回该项的有效缓存数据，并锁定该缓存项（lock = 1）。
**/
static char *
grub_disk_cache_fetch (unsigned long dev_id, unsigned long disk_id,
		       grub_disk_addr_t sector)
{
  struct grub_disk_cache *cache;
  unsigned index;

  index = grub_disk_cache_get_index (dev_id, disk_id, sector);
  cache = grub_disk_cache_table + index;

  if (cache->dev_id == dev_id && cache->disk_id == disk_id
      && cache->sector == sector)
    {
      cache->lock = 1;
#if DISK_CACHE_STATS
      grub_disk_cache_hits++;
#endif
      return cache->data;
    }

#if DISK_CACHE_STATS
  grub_disk_cache_misses++;
#endif

  return 0;
}
/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 解除对应磁盘设备的扇区在disk cache中的cache项的锁定。
*
* @note 注释详细内容:
*
* 本函数实现解除对应磁盘设备的扇区在disk cache中的cache项的锁定的功能。通过调用函数
* grub_disk_cache_get_index()获得在disk cache数组的索引，进而获得对应的cache项，如果
* 该cache项与参数一致（命中），那么就解除锁定该缓存项（lock = 0）。
**/
static void
grub_disk_cache_unlock (unsigned long dev_id, unsigned long disk_id,
			grub_disk_addr_t sector)
{
  struct grub_disk_cache *cache;
  unsigned index;

  index = grub_disk_cache_get_index (dev_id, disk_id, sector);
  cache = grub_disk_cache_table + index;

  if (cache->dev_id == dev_id && cache->disk_id == disk_id
      && cache->sector == sector)
    cache->lock = 0;
}

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 将对应磁盘设备的扇区的数据存储在disk cache中的cache项。
*
* @note 注释详细内容:
*
* 本函数实现将对应磁盘设备的扇区的数据存储在disk cache中的cache项的功能。通过调用函数
* grub_disk_cache_get_index()获得在disk cache数组的索引，进而获得对应的cache项，释放原
* 有cache项的数据，重新分配新的要存储的数据缓冲区，将数据拷贝进入缓冲区，并更新dev_id，
* disk_id以及sector等信息。
**/
static grub_err_t
grub_disk_cache_store (unsigned long dev_id, unsigned long disk_id,
		       grub_disk_addr_t sector, const char *data)
{
  unsigned index;
  struct grub_disk_cache *cache;

  index = grub_disk_cache_get_index (dev_id, disk_id, sector);
  cache = grub_disk_cache_table + index;

  cache->lock = 1;
  grub_free (cache->data);
  cache->data = 0;
  cache->lock = 0;

  cache->data = grub_malloc (GRUB_DISK_SECTOR_SIZE << GRUB_DISK_CACHE_BITS);
  if (! cache->data)
    return grub_errno;

  grub_memcpy (cache->data, data,
	       GRUB_DISK_SECTOR_SIZE << GRUB_DISK_CACHE_BITS);
  cache->dev_id = dev_id;
  cache->disk_id = disk_id;
  cache->sector = sector;

  return GRUB_ERR_NONE;
}

/**< 全局磁盘设备列表 */
grub_disk_dev_t grub_disk_dev_list;

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 注册一个磁盘设备到全局磁盘设备列表中。
*
* @note 注释详细内容:
*
* 本函数实现注册一个磁盘设备到全局磁盘设备列表中的功能。将这个设备放置在该列表的首项。
**/
void
grub_disk_dev_register (grub_disk_dev_t dev)
{
  dev->next = grub_disk_dev_list;
  grub_disk_dev_list = dev;
}

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 从全局磁盘设备列表中删除一个已经注册的磁盘设备。
*
* @note 注释详细内容:
*
* 本函数实现从全局磁盘设备列表中删除一个已经注册的磁盘设备的功能。
**/
void
grub_disk_dev_unregister (grub_disk_dev_t dev)
{
  grub_disk_dev_t *p, q;

  for (p = &grub_disk_dev_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == dev)
      {
        *p = q->next;
	break;
      }
}
/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 查找分区分隔符号（第一个没有用'\'转义的','）的位置。
*
* @note 注释详细内容:
*
* 本函数实现查找分区分隔符号（第一个没有用'\'转义的','）的位置的功能。返回指向','的指针。
**/
/* Return the location of the first ',', if any, which is not
   escaped by a '\'.  */
static const char *
find_part_sep (const char *name)
{
  const char *p = name;
  char c;

  while ((c = *p++) != '\0')
    {
      if (c == '\\' && *p == ',')
	p++;
      else if (c == ',')
	return p - 1;
    }
  return NULL;
}
/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 按照磁盘名字打开磁盘。
*
* @note 注释详细内容:
*
* 本函数实现按照磁盘名字打开磁盘的功能。该函数的大致流程如下：
*
* 1） 使用grub_zalloc()分配一个grub_disk_t结构（变量disk）；
* 2） 使用find_part_sep()看对应名字是否指定了磁盘分区，进而决定磁盘的实际名字（变量raw）；
* 3） 扫描全局磁盘设备列表grub_disk_dev_list，对每个设备，使用raw和disk做参数调用open()；如果
* 该open成功，则算找到对应设备（从而跳出循环）并且打开了该磁盘设备（保存找到的设备disk->dev）；
* 如果没有找到设备或者磁盘扇区大小不被支持，那么就错误退出；
* 4） 如果磁盘设备打开成功，那么看是否还指定了分区，如果又指定了分区，那么就进一步调用函数
* grub_partition_probe()来打开分区，保存返回的分区结构到disk->partition。
* 5） 根据上次访问该磁盘设备到当前访问之间的时间间隔，如果超过2秒钟，那么就无效磁盘cache数据。
* 并更新最后访问该磁盘的时间为本次访问的当前时间。
**/
grub_disk_t
grub_disk_open (const char *name)
{
  const char *p;
  grub_disk_t disk;
  grub_disk_dev_t dev;
  char *raw = (char *) name;
  grub_uint64_t current_time;

  grub_dprintf ("disk", "Opening `%s'...\n", name);

  disk = (grub_disk_t) grub_zalloc (sizeof (*disk));
  if (! disk)
    return 0;
  disk->log_sector_size = GRUB_DISK_SECTOR_BITS;

  p = find_part_sep (name);
  if (p)
    {
      grub_size_t len = p - name;

      raw = grub_malloc (len + 1);
      if (! raw)
	goto fail;

      grub_memcpy (raw, name, len);
      raw[len] = '\0';
      disk->name = grub_strdup (raw);
    }
  else
    disk->name = grub_strdup (name);
  if (! disk->name)
    goto fail;

  for (dev = grub_disk_dev_list; dev; dev = dev->next)
    {
      if ((dev->open) (raw, disk) == GRUB_ERR_NONE)
	break;
      else if (grub_errno == GRUB_ERR_UNKNOWN_DEVICE)
	grub_errno = GRUB_ERR_NONE;
      else
	goto fail;
    }

  if (! dev)
    {
      grub_error (GRUB_ERR_UNKNOWN_DEVICE, N_("disk `%s' not found"),
		  name);
      goto fail;
    }
  if (disk->log_sector_size > GRUB_DISK_CACHE_BITS + GRUB_DISK_SECTOR_BITS
      || disk->log_sector_size < GRUB_DISK_SECTOR_BITS)
    {
      grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		  "sector sizes of %d bytes aren't supported yet",
		  (1 << disk->log_sector_size));
      goto fail;
    }

  disk->dev = dev;

  if (p)
    {
      disk->partition = grub_partition_probe (disk, p + 1);
      if (! disk->partition)
	{
	  /* TRANSLATORS: It means that the specified partition e.g.
	     hd0,msdos1=/dev/sda1 doesn't exist.  */
	  grub_error (GRUB_ERR_UNKNOWN_DEVICE, N_("no such partition"));
	  goto fail;
	}
    }

  /* The cache will be invalidated about 2 seconds after a device was
     closed.  */
  current_time = grub_get_time_ms ();

  if (current_time > (grub_last_time
		      + GRUB_CACHE_TIMEOUT * 1000))
    grub_disk_cache_invalidate_all ();

  grub_last_time = current_time;

 fail:

  if (raw && raw != name)
    grub_free (raw);

  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_error_push ();
      grub_dprintf ("disk", "Opening `%s' failed.\n", name);
      grub_error_pop ();

      grub_disk_close (disk);
      return 0;
    }

  return disk;
}

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 关闭已经打开的磁盘设备。
*
* @note 注释详细内容:
*
* 本函数实现关闭已经打开的磁盘设备的功能。该函数的大致流程如下：
*
* 1）首先调用该磁盘设备的close()函数；
* 2）重置最后访问时间；
* 3）释放该磁盘的所有分区链表的每一项。
* 4）释放该grub_disk_t的名字和结构本身。
**/
void
grub_disk_close (grub_disk_t disk)
{
  grub_partition_t part;
  grub_dprintf ("disk", "Closing `%s'.\n", disk->name);

  if (disk->dev && disk->dev->close)
    (disk->dev->close) (disk);

  /* Reset the timer.  */
  grub_last_time = grub_get_time_ms ();

  while (disk->partition)
    {
      part = disk->partition->parent;
      grub_free (disk->partition);
      disk->partition = part;
    }
  grub_free ((void *) disk->name);
  grub_free (disk);
}
/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 调整磁盘设备扇区范围。
*
* @note 注释详细内容:
*
* 本函数实现调整磁盘设备扇区范围的功能。该函数的大致功能如下：
*
* 1）将扇区号sector从相对于分区改为相对于磁盘（分区相对磁盘的偏移+扇区相对分区的偏移）；
* 2）归一化offset使得其小于扇区大小；
* 3）验证该范围处于分区当中。
**/
/* This function performs three tasks:
   - Make sectors disk relative from partition relative.
   - Normalize offset to be less than the sector size.
   - Verify that the range is inside the partition.  */
static grub_err_t
grub_disk_adjust_range (grub_disk_t disk, grub_disk_addr_t *sector,
			grub_off_t *offset, grub_size_t size)
{
  grub_partition_t part;
  *sector += *offset >> GRUB_DISK_SECTOR_BITS;
  *offset &= GRUB_DISK_SECTOR_SIZE - 1;

  for (part = disk->partition; part; part = part->parent)
    {
      grub_disk_addr_t start;
      grub_uint64_t len;

      start = part->start;
      len = part->len;

      if (*sector >= len
	  || len - *sector < ((*offset + size + GRUB_DISK_SECTOR_SIZE - 1)
			      >> GRUB_DISK_SECTOR_BITS))
	return grub_error (GRUB_ERR_OUT_OF_RANGE,
			   N_("attempt to read or write outside of partition"));

      *sector += start;
    }

  if (disk->total_sectors != GRUB_DISK_SIZE_UNKNOWN
      && ((disk->total_sectors << (disk->log_sector_size - GRUB_DISK_SECTOR_BITS)) <= *sector
	  || ((*offset + size + GRUB_DISK_SECTOR_SIZE - 1)
	  >> GRUB_DISK_SECTOR_BITS) > (disk->total_sectors
				       << (disk->log_sector_size
					   - GRUB_DISK_SECTOR_BITS)) - *sector))
    return grub_error (GRUB_ERR_OUT_OF_RANGE,
		       N_("attempt to read or write outside of disk `%s'"), disk->name);

  return GRUB_ERR_NONE;
}

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 转换磁盘设备扇区编号。
*
* @note 注释详细内容:
*
* 本函数实现转换磁盘设备扇区编号的功能。从512字节模式扇区编号转换为实际扇区大小模式的扇区
* 编号。实际使用的是运算”sector >> (disk->log_sector_size - GRUB_DISK_SECTOR_BITS)“；例如，
* 如果本来磁盘扇区大小实际就是512字节，那么disk->log_sector_size=9，与GRUB_DISK_SECTOR_BITS
* 一样，因此返回值就是sector本身。而加入磁盘的实际扇区大小是4KB，即disk->log_sector_size=12，
* 那么返回的值就是（sector>>3），因此，就是原来的山区编号0~7都会是0，原来的8~15都会是1，以
* 此类推。
**/
static inline grub_disk_addr_t
transform_sector (grub_disk_t disk, grub_disk_addr_t sector)
{
  return sector >> (disk->log_sector_size - GRUB_DISK_SECTOR_BITS);
}

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 读取较小的磁盘扇区数据（小于cache size并且不越过cache项边界）。
*
* @note 注释详细内容:
*
* 本函数实现读取较小的磁盘扇区数据（小于cache size并且不越过cache项边界）的功能。大致
* 步骤如下：
*
* 1）首先调用grub_disk_cache_fetch()，如果命中，那么直接返回缓存的数据。
* 2）如果不命中，那么尝试直接实际读取cahce块大小的数据，并将之存储到缓存；
* 3）如果读取cache块大小数据也失败了，那么再次尝试读取请求的实际大小。
**/
/* Small read (less than cache size and not pass across cache unit boundaries).
   sector is already adjusted and is divisible by cache unit size.
 */
static grub_err_t
grub_disk_read_small (grub_disk_t disk, grub_disk_addr_t sector,
		      grub_off_t offset, grub_size_t size, void *buf)
{
  char *data;
  char *tmp_buf;

  /* Fetch the cache.  */
  data = grub_disk_cache_fetch (disk->dev->id, disk->id, sector);
  if (data)
    {
      /* Just copy it!  */
      grub_memcpy (buf, data + offset, size);
      grub_disk_cache_unlock (disk->dev->id, disk->id, sector);
      return GRUB_ERR_NONE;
    }

  /* Allocate a temporary buffer.  */
  tmp_buf = grub_malloc (GRUB_DISK_SECTOR_SIZE << GRUB_DISK_CACHE_BITS);
  if (! tmp_buf)
    return grub_errno;

  /* Otherwise read data from the disk actually.  */
  if (disk->total_sectors == GRUB_DISK_SIZE_UNKNOWN
      || sector + GRUB_DISK_CACHE_SIZE/**< 读取一个cache块不会超过磁盘总大小末尾 */
      < (disk->total_sectors << (disk->log_sector_size - GRUB_DISK_SECTOR_BITS)))
    {
      grub_err_t err;
      err = (disk->dev->read) (disk, transform_sector (disk, sector),
			       1 << (GRUB_DISK_CACHE_BITS
				     + GRUB_DISK_SECTOR_BITS
				     - disk->log_sector_size), tmp_buf);
      if (!err)
	{
	  /* Copy it and store it in the disk cache.  */
	  grub_memcpy (buf, tmp_buf + offset, size);
	  grub_disk_cache_store (disk->dev->id, disk->id,
				 sector, tmp_buf);
	  grub_free (tmp_buf);
	  return GRUB_ERR_NONE;
	}
    }

  grub_free (tmp_buf);
  grub_errno = GRUB_ERR_NONE;

  {
    /* Uggh... Failed. Instead, just read necessary data.  */
    unsigned num;
    grub_disk_addr_t aligned_sector;

    sector += (offset >> GRUB_DISK_SECTOR_BITS);/**< 归一化磁盘sector和offset */
    offset &= ((1 << GRUB_DISK_SECTOR_BITS) - 1);
    aligned_sector = (sector & ~((1 << (disk->log_sector_size
					- GRUB_DISK_SECTOR_BITS))
				 - 1));
    offset += ((sector - aligned_sector) << GRUB_DISK_SECTOR_BITS);
    num = ((size + offset + (1 << (disk->log_sector_size))
	    - 1) >> (disk->log_sector_size));

    tmp_buf = grub_malloc (num << disk->log_sector_size);
    if (!tmp_buf)
      return grub_errno;

    if ((disk->dev->read) (disk, transform_sector (disk, aligned_sector),
			   num, tmp_buf))
      {
	grub_error_push ();
	grub_dprintf ("disk", "%s read failed\n", disk->name);
	grub_error_pop ();
	grub_free (tmp_buf);
	return grub_errno;
      }
    grub_memcpy (buf, tmp_buf + offset, size);
    grub_free (tmp_buf);
    return GRUB_ERR_NONE;
  }
}
/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 读取磁盘扇区数据。
*
* @note 注释详细内容:
*
* 本函数实现读取磁盘扇区数据的功能。该函数大致步骤如下：
*
* 1）首先调用grub_disk_adjust_range()，归一化sector和offset。
* 2）如果起始扇区sector和偏移offset没有cache块对其，那么将最先部分没有cahce块对其的部分
* 使用grub_disk_read_small()读取出来；
* 3）剩余的数据分为两部分，首先对于中间的整数cache块大小的，按照cache块大小读入，并存储
* 到cache块中。
* 4）在最后，如果还有不足cache块大小的数据，再次调用grub_disk_read_small()读取出来。
* 5）调用read hook钩子函数。
**/
/* Read data from the disk.  */
grub_err_t
grub_disk_read (grub_disk_t disk, grub_disk_addr_t sector,
		grub_off_t offset, grub_size_t size, void *buf)
{
  grub_off_t real_offset;
  grub_disk_addr_t real_sector;
  grub_size_t real_size;

  /* First of all, check if the region is within the disk.  */
  if (grub_disk_adjust_range (disk, &sector, &offset, size) != GRUB_ERR_NONE)
    {
      grub_error_push ();
      grub_dprintf ("disk", "Read out of range: sector 0x%llx (%s).\n",
		    (unsigned long long) sector, grub_errmsg);
      grub_error_pop ();
      return grub_errno;
    }

  real_sector = sector;
  real_offset = offset;
  real_size = size;

  /* First read until first cache boundary.   */
  if (offset || (sector & (GRUB_DISK_CACHE_SIZE - 1)))
    {
      grub_disk_addr_t start_sector;
      grub_size_t pos;
      grub_err_t err;
      grub_size_t len;

      start_sector = sector & ~(GRUB_DISK_CACHE_SIZE - 1);
      pos = (sector - start_sector) << GRUB_DISK_SECTOR_BITS;
      len = ((GRUB_DISK_SECTOR_SIZE << GRUB_DISK_CACHE_BITS)
	     - pos - offset);
      if (len > size)
	len = size;
      err = grub_disk_read_small (disk, start_sector,
				  offset + pos, len, buf);
      if (err)
	return err;
      buf = (char *) buf + len;
      size -= len;
      offset += len;
      sector += (offset >> GRUB_DISK_SECTOR_BITS);
      offset &= ((1 << GRUB_DISK_SECTOR_BITS) - 1);
    }

  /* Until SIZE is zero...  */
  while (size >= (GRUB_DISK_CACHE_SIZE << GRUB_DISK_SECTOR_BITS))
    {
      char *data = NULL;
      grub_disk_addr_t agglomerate;
      grub_err_t err;

      /* agglomerate read until we find a first cached entry.  */
      for (agglomerate = 0; agglomerate
	     < (size >> (GRUB_DISK_SECTOR_BITS + GRUB_DISK_CACHE_BITS));
	   agglomerate++)
	{
	  data = grub_disk_cache_fetch (disk->dev->id, disk->id,
					sector + (agglomerate
						  << GRUB_DISK_CACHE_BITS));
	  if (data)
	    break;
	}

      if (data)
	{
	  grub_memcpy ((char *) buf
		       + (agglomerate << (GRUB_DISK_CACHE_BITS
					  + GRUB_DISK_SECTOR_BITS)),
		       data, GRUB_DISK_CACHE_SIZE << GRUB_DISK_SECTOR_BITS);
	  grub_disk_cache_unlock (disk->dev->id, disk->id,
				  sector + (agglomerate
					    << GRUB_DISK_CACHE_BITS));
	}

      if (agglomerate)
	{
	  grub_disk_addr_t i;

	  err = (disk->dev->read) (disk, transform_sector (disk, sector),
				   agglomerate << (GRUB_DISK_CACHE_BITS
						   + GRUB_DISK_SECTOR_BITS
						   - disk->log_sector_size),
				   buf);
	  if (err)
	    return err;

	  for (i = 0; i < agglomerate; i ++)
	    grub_disk_cache_store (disk->dev->id, disk->id,
				   sector + (i << GRUB_DISK_CACHE_BITS),
				   (char *) buf
				   + (i << (GRUB_DISK_CACHE_BITS
					    + GRUB_DISK_SECTOR_BITS)));

	  sector += agglomerate << GRUB_DISK_CACHE_BITS;
	  size -= agglomerate << (GRUB_DISK_CACHE_BITS + GRUB_DISK_SECTOR_BITS);
	  buf = (char *) buf
	    + (agglomerate << (GRUB_DISK_CACHE_BITS + GRUB_DISK_SECTOR_BITS));
	}

      if (data)
	{
	  sector += GRUB_DISK_CACHE_SIZE;
	  buf = (char *) buf + (GRUB_DISK_CACHE_SIZE << GRUB_DISK_SECTOR_BITS);
	  size -= (GRUB_DISK_CACHE_SIZE << GRUB_DISK_SECTOR_BITS);
	}
    }

  /* And now read the last part.  */
  if (size)
    {
      grub_err_t err;
      err = grub_disk_read_small (disk, sector, 0, size, buf);
      if (err)
	return err;
    }

  /* Call the read hook, if any.  */
  if (disk->read_hook)
    {
      grub_disk_addr_t s = real_sector;
      grub_size_t l = real_size;
      grub_off_t o = real_offset;

      while (l)
	{
	  grub_size_t cl;
	  cl = GRUB_DISK_SECTOR_SIZE - o;
	  if (cl > l)
	    cl = l;
	  (disk->read_hook) (s, o, cl);
	  s++;
	  l -= cl;
	  o = 0;
	}
    }

  return grub_errno;
}

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 写入磁盘扇区数据。
*
* @note 注释详细内容:
*
* 本函数实现读取磁盘扇区数据的功能。该函数大致步骤如下：
*
* 1）首先调用grub_disk_adjust_range()，归一化sector和offset。
* 2）计算aligned_sector，将sector对齐到前一个实际扇区大小处；计算real_offset为相对于前面
* 的aligned_sector的偏移；并将操作的sector设置到aligned_sector。
* 3）将要写入的数据分成3个部分，按照下面的情况，使用一个循环完成实际的写入：
* 3.1) 如果开始部分没有与实际扇区大小对应，那么real_offset！= 0，则先调用grub_disk_read()
* 读入一个实际扇区大小的数据，然后将前面部分要写入的但是又没有与实际扇区大小对齐的部分
* 合并，然后再将合并后的数据写入；同事无效这部分磁盘cache；
* 3.2）中间部分，按照整个扇区大小的整数倍，全部写入；对应的磁盘cache也被无效；
* 3.3) 剩余的部分，如果还有不足实际扇区大小的数据，也按照3.1）的办法，先读入，再合并，
* 再无效cache，然后再将合并后的数据写入磁盘。
**/
grub_err_t
grub_disk_write (grub_disk_t disk, grub_disk_addr_t sector,
		 grub_off_t offset, grub_size_t size, const void *buf)
{
  unsigned real_offset;
  grub_disk_addr_t aligned_sector;

  grub_dprintf ("disk", "Writing `%s'...\n", disk->name);

  if (grub_disk_adjust_range (disk, &sector, &offset, size) != GRUB_ERR_NONE)
    return -1;

  aligned_sector = (sector & ~((1 << (disk->log_sector_size
				      - GRUB_DISK_SECTOR_BITS)) - 1));
  real_offset = offset + ((sector - aligned_sector) << GRUB_DISK_SECTOR_BITS);
  sector = aligned_sector;

  while (size)
    {
      if (real_offset != 0 || (size < (1U << disk->log_sector_size)
			       && size != 0))
	{
	  char tmp_buf[1 << disk->log_sector_size];
	  grub_size_t len;
	  grub_partition_t part;

	  part = disk->partition;
	  disk->partition = 0;
	  if (grub_disk_read (disk, sector,
			      0, (1 << disk->log_sector_size), tmp_buf)
	      != GRUB_ERR_NONE)
	    {
	      disk->partition = part;
	      goto finish;
	    }
	  disk->partition = part;

	  len = (1 << disk->log_sector_size) - real_offset;
	  if (len > size)
	    len = size;

	  grub_memcpy (tmp_buf + real_offset, buf, len);

	  grub_disk_cache_invalidate (disk->dev->id, disk->id, sector);

	  if ((disk->dev->write) (disk, sector, 1, tmp_buf) != GRUB_ERR_NONE)
	    goto finish;

	  sector += (1 << (disk->log_sector_size - GRUB_DISK_SECTOR_BITS));
	  buf = (const char *) buf + len;
	  size -= len;
	  real_offset = 0;
	}
      else
	{
	  grub_size_t len;
	  grub_size_t n;

	  len = size & ~((1 << disk->log_sector_size) - 1);
	  n = size >> disk->log_sector_size;

	  if ((disk->dev->write) (disk, sector, n, buf) != GRUB_ERR_NONE)
	    goto finish;

	  while (n--)
	    grub_disk_cache_invalidate (disk->dev->id, disk->id, sector++);

	  buf = (const char *) buf + len;
	  size -= len;
	}
    }

 finish:

  return grub_errno;
}
/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 获取磁盘的容量大小。
*
* @note 注释详细内容:
*
* 本函数实现获取磁盘的容量大小的功能。如果是分区，则调用grub_partition_get_len()；如果磁盘
* total_sectors已知，则返回实际的磁盘大小；否则返回GRUB_DISK_SIZE_UNKNOWN。
**/
grub_uint64_t
grub_disk_get_size (grub_disk_t disk)
{
  if (disk->partition)
    return grub_partition_get_len (disk->partition);
  else if (disk->total_sectors != GRUB_DISK_SIZE_UNKNOWN)
    return disk->total_sectors << (disk->log_sector_size - GRUB_DISK_SECTOR_BITS);
  else
    return GRUB_DISK_SIZE_UNKNOWN;
}
