/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004,2007  Free Software Foundation, Inc.
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

#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/partition.h>
#include <grub/disk.h>
#include <grub/i18n.h>

#ifdef GRUB_UTIL
#include <grub/util/misc.h>
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
* @brief GRUB2的磁盘设备与分区命名规范简介。
*
* @note 注释详细内容:
*
* GRUB2用的磁盘设备语法相对与以前的GRUB Legacy发生了重要变化。 例如下面的软盘设备名：
* 
* (fd0) 
* 
* 首先 GRUB2 需要设备名称被扩在圆括号内，fd 表示软盘，数字0表示编号为0的设备（第一个软
* 盘设备），编号从零开始计数。 再有：
* 
* (hd0,msdos2) 
* 
* hd 意思是硬盘，数字0代表设备号，意味着是第一块硬盘。Msdos 指出了分区类型，数字2代表
* 分区编号。分区编号从1开始计数，而不是0。因此前面的代码指定了第一个硬盘的第二个分区为
* msdos格式。
*  
* 当你选择了分区时 GRUB 会尝试解析文件系统，并从分区中读取资料。 
* 
* (hd0,msdos5) 
* 
* 这指定了第一个硬盘的第一个扩展分区。由于主分区最多为 4，所以扩展分区从 5 开始编号。 
* 
* (hd1,msdos1,bsd1) 
* 
* 第 2 块硬盘的第一个BSD slice（BSD使用slice管理磁盘，概念类似于主分区）。 
* 
* 如果想让GRUB2真实的访问磁盘和分区，就需要在命令行中按照上述语法指定分区。例如： 
* 
* set root=(fd0) 
* 
* parttool (hd0,msdos3) hidden- 
* 
* 如果不知道磁盘中的分区方法，可以使用GRUB2的Tab补全功能。只输入 
* 
* set root=( 
* 
* 然后按下 Tab 键，GRUB2 会自动显示设备列表，分区名，文件名。 
* 
* 注意GRUB2不把SCSI区别于IDE，它简单的从0开始给设备编号，而忽略设备类型。一般IDE设备
* 的编号小于SCSI设备。如果你在BIOS中改变了IDE和SCSI的引导顺序，那编号大小就说不准了。 
* 
* 现在考虑一个问题，如何指定一个文件： 
* 
* (hd0,msdos1)/vmlinuz 
* 
* 它在指定分区(hd0,msdos1)上指定文件vmlinuz。依次类推，可以指定任意路径。 
**/

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
* @note 注释详细内容:
* 
* 分区管理器有一个分区映射类型链表，用来管理磁盘中的分区。同一映射类型中可能会有好几个分
* 区，分区内又有分区映射类型和子分区。为了处理这样的分区分布，而有分区与分区映射类型的相
* 关函数。
**/
grub_partition_map_t grub_partition_map_list;
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
* @brief 检查磁盘disk是否包含分区part。
*
* @note 注释详细内容:
*
* 本函检查磁盘disk是否包含分区part的功能。如果part->start + part->len比disk->partition->len
* 大，那么就是disk没有完全包含。
**/
/*
 * Checks that disk->partition contains part.  This function assumes that the
 * start of part is relative to the start of disk->partition.  Returns 1 if
 * disk->partition is null.
 */
static int
grub_partition_check_containment (const grub_disk_t disk,
				  const grub_partition_t part)
{
  if (disk->partition == NULL)
    return 1;

  if (part->start + part->len > disk->partition->len)
    {
      char *partname;

      partname = grub_partition_get_name (disk->partition);
      grub_dprintf ("partition", "sub-partition %s%d of (%s,%s) ends after parent.\n",
		    part->partmap->name, part->number + 1, disk->name, partname);
#ifdef GRUB_UTIL
      grub_util_warn (_("Discarding improperly nested partition (%s,%s,%s%d)"),
		      disk->name, partname, part->partmap->name, part->number + 1);
#endif
      grub_free (partname);

      return 0;
    }

  return 1;
}
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
* @brief 探测磁盘的分区映射信息。
*
* @note 注释详细内容:
*
* 本函检查探测磁盘的分区映射信息的功能。
**/
static grub_partition_t
grub_partition_map_probe (const grub_partition_map_t partmap,
			  grub_disk_t disk, int partnum)
{
  grub_partition_t p = 0;

  auto int find_func (grub_disk_t d, const grub_partition_t partition);

  int find_func (grub_disk_t dsk,
		 const grub_partition_t partition)
    {
      if (partnum != partition->number)
	return 0;

      if (!(grub_partition_check_containment (dsk, partition)))
	return 0;

      p = (grub_partition_t) grub_malloc (sizeof (*p));
      if (! p)
	return 1;

      grub_memcpy (p, partition, sizeof (*p));
      return 1;
    }

  partmap->iterate (disk, find_func);
  if (grub_errno)
    goto fail;

  return p;

 fail:
  grub_free (p);
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
* @date 注释添加日期：2013年6月21日
*
* @brief 探测磁盘的分区信息。
*
* @note 注释详细内容:
*
* 本函检查探测磁盘的分区信息的功能。每种磁盘分区架构都会有一个分区映射解析文件，例如
* msdos，apple等，相关代码在grub-2.00/grub-core/partmap中。每一个分区映射架构都会使用
* 函数grub_partition_map_register()注册一个struct grub_partition_map；例如，对于GPT分
* 区架构，就在grub-2.00/grub-core/partmap/gpt.c中注册了grub_gpt_partition_map。该函数
* 使用一个for循环，将参数str按照','分开，每一个分开的部分都认为是一个分区的名字（包括
* 分区名字partname和分区号num）；对于每个分区部分，都使用FOR_PARTITION_MAPS()来扫描整
* 个grub_partition_map链表，如果分区名partname和partmap->name一致，则调用分区映射探测
* 函数grub_partition_map_probe()来探测该分区；如果该函数返回一个grub_partition_t，那、
* 么认为这个分区的确属于这种分区映射，从而跳出FOR_PARTITION_MAPS()循环;否则，如果返回
* 的是GRUB_ERR_BAD_PART_TABLE，那么就重新尝试下一个grub_partition_map类型。每次识别一
* 个新的分区，都将前一个分区作为新识别的分区的parent；该函数返回最后识别到的分区。
**/
grub_partition_t
grub_partition_probe (struct grub_disk *disk, const char *str)
{
  grub_partition_t part = 0;
  grub_partition_t curpart = 0;
  grub_partition_t tail;
  const char *ptr;

  part = tail = disk->partition;

  for (ptr = str; *ptr;)
    {
      grub_partition_map_t partmap;
      int num;
      const char *partname, *partname_end;

      partname = ptr;
      while (*ptr && grub_isalpha (*ptr))
	ptr++;
      partname_end = ptr; 
      num = grub_strtoul (ptr, (char **) &ptr, 0) - 1;

      curpart = 0;
      /* Use the first partition map type found.  */
      FOR_PARTITION_MAPS(partmap)
      {
	if (partname_end != partname &&
	    (grub_strncmp (partmap->name, partname, partname_end - partname)
	     != 0 || partmap->name[partname_end - partname] != 0))
	  continue;

	disk->partition = part;
	curpart = grub_partition_map_probe (partmap, disk, num);
	disk->partition = tail;
	if (curpart)
	  break;

	if (grub_errno == GRUB_ERR_BAD_PART_TABLE)
	  {
	    /* Continue to next partition map type.  */
	    grub_errno = GRUB_ERR_NONE;
	    continue;
	  }

	break;
      }

      if (! curpart)
	{
	  while (part)
	    {
	      curpart = part->parent;
	      grub_free (part);
	      part = curpart;
	    }
	  return 0;
	}
      curpart->parent = part;
      part = curpart;
      if (! ptr || *ptr != ',')
	break;
      ptr++;
    }

  return part;
}
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
* @brief 重复递归地对磁盘的每个分区调用一个回调函数。
*
* @note 注释详细内容:
*
* 本函实现重复递归地对磁盘的每个分区调用一个回调函数的功能。函数首先用FOR_PARTITION_MAPS()
* 来对每个grub_partition_map链表项partmap调用partmap->iterate()，而这个过程中的回调函数是
* 内部函数part_iterate()；这个part_iterate()首先检查该分区是否属于该磁盘，如果是的话，就对
* 其调用hook()回调函数；如果该分区的起始地址不是0，那么继续使用FOR_PARTITION_MAPS()对磁盘
* 的当前分区进行处理，每次都用partmap->iterate()来递归调用part_iterate()。这样，就可以实现
* 对磁盘的每个分区都调用hook()回调函数。探知分区是否正确的存在于磁盘的步骤是探知磁盘的分区
* 和分区映射类型的分区。主要三个函数grub_partition_iterate、partmap->iterate、part_iterate。
* grub_partition_iterate调用partmap->iterate，目的是检验磁盘中的分区。partmap->iterate会调
* 用part_iterate，part_iterate 再调用grub_partition_iterate 的hook函数，iterate_partition、
* find_partition、identify_partmap，用于检验磁盘分区。
**/
int
grub_partition_iterate (struct grub_disk *disk,
			int (*hook) (grub_disk_t disk,
				     const grub_partition_t partition))
{
  int ret = 0;

  auto int part_iterate (grub_disk_t dsk, const grub_partition_t p);

  int part_iterate (grub_disk_t dsk,
		    const grub_partition_t partition)
    {
      struct grub_partition p = *partition;

      if (!(grub_partition_check_containment (dsk, partition)))
	return 0;

      p.parent = dsk->partition;
      dsk->partition = 0;
      if (hook (dsk, &p))
	{
	  ret = 1;
	  return 1;
	}
      if (p.start != 0)
	{
	  const struct grub_partition_map *partmap;
	  dsk->partition = &p;
	  FOR_PARTITION_MAPS(partmap)
	  {
	    grub_err_t err;
	    err = partmap->iterate (dsk, part_iterate);
	    if (err)
	      grub_errno = GRUB_ERR_NONE;
	    if (ret)
	      break;
	  }
	}
      dsk->partition = p.parent;
      return ret;
    }

  {
    const struct grub_partition_map *partmap;
    FOR_PARTITION_MAPS(partmap)
    {
      grub_err_t err;
      err = partmap->iterate (disk, part_iterate);
      if (err)
	grub_errno = GRUB_ERR_NONE;
      if (ret)
	break;
    }
  }

  return ret;
}
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
* @brief 获得分区名字。
*
* @note 注释详细内容:
*
* 本函实现获得分区名字的功能。函数整体是一个分区名称字串取得循环，从该分区往上找父分区，
* 直到磁盘的主分区为止。这样就可以取得完整的分区名称。取得分区的名称字串，这个名称字符
* 串是从磁盘的主分区开始算起的名称字符串。也就是说，分区的名称字符串是从磁盘的主分区算
* 起的名称字符串，并且以逗号分开。
**/
char *
grub_partition_get_name (const grub_partition_t partition)
{
  char *out = 0, *ptr;
  grub_size_t needlen = 0;
  grub_partition_t part;
  if (!partition)
    return grub_strdup ("");
  for (part = partition; part; part = part->parent)
    /* Even on 64-bit machines this buffer is enough to hold
       longest number.  */
    needlen += grub_strlen (part->partmap->name) + 1 + 27;
  out = grub_malloc (needlen + 1);
  if (!out)
    return NULL;

  ptr = out + needlen;
  *ptr = 0;
  for (part = partition; part; part = part->parent)
    {
      char buf[27];
      grub_size_t len;
      grub_snprintf (buf, sizeof (buf), "%d", part->number + 1);
      len = grub_strlen (buf);
      ptr -= len;
      grub_memcpy (ptr, buf, len);
      len = grub_strlen (part->partmap->name);
      ptr -= len;
      grub_memcpy (ptr, part->partmap->name, len);
      *--ptr = ',';
    }
  grub_memmove (out, ptr + 1, out + needlen - ptr);
  return out;
}
