/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2004,2006,2007  Free Software Foundation, Inc.
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

#ifndef GRUB_PART_HEADER
#define GRUB_PART_HEADER	1

#include <grub/dl.h>
#include <grub/list.h>

struct grub_disk;

typedef struct grub_partition *grub_partition_t;

#ifdef GRUB_UTIL
typedef enum
{
  GRUB_EMBED_PCBIOS
} grub_embed_type_t;
#endif

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月21日
*
* @brief 分区管理器介绍
*
* @param next 分区映射类型指针，指向下一个分区映射类型。
* @param prev 分区映射类型指针，指向前一个分区映射类型。
* @param name 分区映射类型名称字符串指针，指向此分区映射类型的字符串位址。
* @param iterate 分区反覆探测函数指针，指向该分区映射类型专用的函数。
**/
/* Partition map type.  */
struct grub_partition_map
{
  /* The next partition map type.  */
  struct grub_partition_map *next;
  struct grub_partition_map **prev;

  /* The name of the partition map type.  */
  const char *name;

  /* Call HOOK with each partition, until HOOK returns non-zero.  */
  grub_err_t (*iterate) (struct grub_disk *disk,
			 int (*hook) (struct grub_disk *disk,
				      const grub_partition_t partition));
#ifdef GRUB_UTIL
  /* Determine sectors available for embedding.  */
  grub_err_t (*embed) (struct grub_disk *disk, unsigned int *nsectors,
		       unsigned int max_nsectors,
		       grub_embed_type_t embed_type,
		       grub_disk_addr_t **sectors);
#endif
};
typedef struct grub_partition_map *grub_partition_map_t;
/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月21日
*
* @brief 分区管理器介绍
*
* @param number 分区号码，。
* @param start 分区磁盘起始磁区位置，。
* @param len 分区长度，单位是扇区。
* @param offset 分区偏移。
* @param index 分区表项索引。
* @param parent 主分区指针，不为空指针时，表示此分区为子分区。
* @param partmap 分区映射类型，。
* @param msdostype 分区类型，。
**/
/* Partition description.  */
struct grub_partition
{
  /* The partition number.  */
  int number;

  /* The start sector (relative to parent).  */
  grub_disk_addr_t start;

  /* The length in sector units.  */
  grub_uint64_t len;

  /* The offset of the partition table.  */
  grub_disk_addr_t offset;

  /* The index of this partition in the partition table.  */
  int index;

  /* Parent partition (physically contains this partition).  */
  struct grub_partition *parent;

  /* The type partition map.  */
  grub_partition_map_t partmap;

  /* The type of partition whne it's on MSDOS.
     Used for embedding detection.  */
  grub_uint8_t msdostype;
};

grub_partition_t EXPORT_FUNC(grub_partition_probe) (struct grub_disk *disk,
						    const char *str);
int EXPORT_FUNC(grub_partition_iterate) (struct grub_disk *disk,
					 int (*hook) (struct grub_disk *disk,
						      const grub_partition_t partition));
char *EXPORT_FUNC(grub_partition_get_name) (const grub_partition_t partition);


extern grub_partition_map_t EXPORT_VAR(grub_partition_map_list);

#ifndef GRUB_LST_GENERATOR
static inline void
grub_partition_map_register (grub_partition_map_t partmap)
{
  grub_list_push (GRUB_AS_LIST_P (&grub_partition_map_list),
		  GRUB_AS_LIST (partmap));
}
#endif

static inline void
grub_partition_map_unregister (grub_partition_map_t partmap)
{
  grub_list_remove (GRUB_AS_LIST (partmap));
}

#define FOR_PARTITION_MAPS(var) FOR_LIST_ELEMENTS((var), (grub_partition_map_list))


static inline grub_disk_addr_t
grub_partition_get_start (const grub_partition_t p)
{
  grub_partition_t part;
  grub_uint64_t part_start = 0;

  for (part = p; part; part = part->parent)
    part_start += part->start;

  return part_start;
}

static inline grub_uint64_t
grub_partition_get_len (const grub_partition_t p)
{
  return p->len;
}

#endif /* ! GRUB_PART_HEADER */
