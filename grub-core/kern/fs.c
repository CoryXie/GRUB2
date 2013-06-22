/* fs.c - filesystem manager */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2005,2007  Free Software Foundation, Inc.
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
#include <grub/net.h>
#include <grub/fs.h>
#include <grub/file.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/types.h>
#include <grub/mm.h>
#include <grub/term.h>
#include <grub/i18n.h>

grub_fs_t grub_fs_list = 0;

grub_fs_autoload_hook_t grub_fs_autoload_hook = 0;
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
* @brief 探测设备的文件系统。
*
* @note 注释详细内容:
*
* 本函数实现探测设备的文件系统的功能。分两种情况：
*
* 1） 如果是网络设备，则直接返回device->net->fs；
* 2） 然而如果该设备是磁盘设备(device->disk非空)，则该函数首先扫描目前已经载入了的文件系
* 统列表grub_fs_list，使用每个可支持的文件系统的p->dir()函数来测试是否可以加载该设备，如
* 果可以（grub_errno返回GRUB_ERR_NONE），则返回该grub_fs_list文件系统项；接着如果目前支持
* grub_fs_autoload_hook（非空），那么就循环调用grub_fs_autoload_hook()，直到该函数返回0。
* 在调用grub_fs_autoload_hook()的过程中（实际上调用的是grub-2.00/grub-core/normal/autofs.c
* 中的autoload_fs_module()函数），会根据fs_module_list来加载文件系统支持模块，每次加入一个
* 文件系统的支持，该支持模块就使用grub_fs_register()加入一个grub_fs_t项到grub_fs_list的首部；
* 也就是说grub_fs_list一直是刚刚加入的那个文件系统模块；因此，调用完grub_fs_autoload_hook()
* 之后，代码立即使用grub_fs_list（也就是刚刚载入的文件系统支持模块）本身的->dir()函数来尝试
* 是否可以加载该设备。也就是说，该函数在探测磁盘文件系统的时候，不但使用了已经载入到系统
* 中的文件系统，还在已经载入的文件系统都没探测成功的时候尝试用尚未载入的文件系统支持模块
* 来看是否可以支持，也就是说支持了文件系统的自动加载机制。
**/
grub_fs_t
grub_fs_probe (grub_device_t device)
{
  grub_fs_t p;
  auto int dummy_func (const char *filename,
		       const struct grub_dirhook_info *info);

  int dummy_func (const char *filename __attribute__ ((unused)),
		  const struct grub_dirhook_info *info  __attribute__ ((unused)))
    {
      return 1;
    }

  if (device->disk)
    {
      /* Make it sure not to have an infinite recursive calls.  */
      static int count = 0;

      for (p = grub_fs_list; p; p = p->next)
	{
	  grub_dprintf ("fs", "Detecting %s...\n", p->name);

	  /* This is evil: newly-created just mounted BtrFS after copying all
	     GRUB files has a very peculiar unrecoverable corruption which
	     will be fixed at sync but we'd rather not do a global sync and
	     syncing just files doesn't seem to help. Relax the check for
	     this time.  */
#ifdef GRUB_UTIL
	  if (grub_strcmp (p->name, "btrfs") == 0)
	    {
	      char *label = 0;
	      p->uuid (device, &label);
	      if (label)
		grub_free (label);
	    }
	  else
#endif
	    (p->dir) (device, "/", dummy_func);
	  if (grub_errno == GRUB_ERR_NONE)
	    return p;

	  grub_error_push ();
	  grub_dprintf ("fs", "%s detection failed.\n", p->name);
	  grub_error_pop ();

	  if (grub_errno != GRUB_ERR_BAD_FS
	      && grub_errno != GRUB_ERR_OUT_OF_RANGE)
	    return 0;

	  grub_errno = GRUB_ERR_NONE;
	}

      /* Let's load modules automatically.  */
      if (grub_fs_autoload_hook && count == 0)
	{
	  count++;

	  while (grub_fs_autoload_hook ())
	    {
	      p = grub_fs_list;

	      (p->dir) (device, "/", dummy_func);
	      if (grub_errno == GRUB_ERR_NONE)
		{
		  count--;
		  return p;
		}

	      if (grub_errno != GRUB_ERR_BAD_FS
		  && grub_errno != GRUB_ERR_OUT_OF_RANGE)
		{
		  count--;
		  return 0;
		}

	      grub_errno = GRUB_ERR_NONE;
	    }

	  count--;
	}
    }
  else if (device->net && device->net->fs)
    return device->net->fs;

  grub_error (GRUB_ERR_UNKNOWN_FS, N_("unknown filesystem"));
  return 0;
}



/* Block list support routines.  */

struct grub_fs_block
{
  grub_disk_addr_t offset;
  unsigned long length;
};
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
* @brief 按照block list的方式打开文件。
*
* @note 注释详细内容:
*
* 本函数实现按照block list的方式打开文件的功能。文件名参数name是以如下格式的字符串数组：
* "offset+length,offset+length,..."；该函数首先通过计算文件名中的","个数来判断该文件有多
* 少个block，然后分配这么多个struct grub_fs_block的数组空间；对每个struct grub_fs_block再
* 解析offset和length来初始化；并计算该文件总体的大小，保存在file->size中；最后将生成的
* struct grub_fs_block的数组空间指针保存在file->data中。
**/
static grub_err_t
grub_fs_blocklist_open (grub_file_t file, const char *name)
{
  char *p = (char *) name;
  unsigned num = 0;
  unsigned i;
  grub_disk_t disk = file->device->disk;
  struct grub_fs_block *blocks;

  /* First, count the number of blocks.  */
  do
    {
      num++;
      p = grub_strchr (p, ',');
      if (p)
	p++;
    }
  while (p);

  /* Allocate a block list.  */
  blocks = grub_zalloc (sizeof (struct grub_fs_block) * (num + 1));
  if (! blocks)
    return 0;

  file->size = 0;
  p = (char *) name;
  for (i = 0; i < num; i++)
    {
      if (*p != '+')
	{
	  blocks[i].offset = grub_strtoull (p, &p, 0);
	  if (grub_errno != GRUB_ERR_NONE || *p != '+')
	    {
	      grub_error (GRUB_ERR_BAD_FILENAME,
			  N_("invalid file name `%s'"), name);
	      goto fail;
	    }
	}

      p++;
      blocks[i].length = grub_strtoul (p, &p, 0);
      if (grub_errno != GRUB_ERR_NONE
	  || blocks[i].length == 0
	  || (*p && *p != ',' && ! grub_isspace (*p)))
	{
	  grub_error (GRUB_ERR_BAD_FILENAME,
		      N_("invalid file name `%s'"), name);
	  goto fail;
	}

      if (disk->total_sectors < blocks[i].offset + blocks[i].length)
	{
	  grub_error (GRUB_ERR_BAD_FILENAME, "beyond the total sectors");
	  goto fail;
	}

      file->size += (blocks[i].length << GRUB_DISK_SECTOR_BITS);
      p++;
    }

  file->data = blocks;

  return GRUB_ERR_NONE;

 fail:
  grub_free (blocks);
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
* @brief 按照block list的方式读取文件。
*
* @note 注释详细内容:
*
* 本函数实现按照block list的方式读取文件的功能。首先根据file->offset活动文件当前偏移的
* sector和sector offset；文件可以是一个block或很多个block，每个block都有磁盘扇区位址和
* 磁盘扇区长度。读取文件时，将文件偏移转换成磁盘的磁盘扇区位址和磁盘扇区长度，再执行读
* 取。
**/
static grub_ssize_t
grub_fs_blocklist_read (grub_file_t file, char *buf, grub_size_t len)
{
  struct grub_fs_block *p;
  grub_disk_addr_t sector;
  grub_off_t offset;
  grub_ssize_t ret = 0;

  if (len > file->size - file->offset)
    len = file->size - file->offset;

  sector = (file->offset >> GRUB_DISK_SECTOR_BITS);
  offset = (file->offset & (GRUB_DISK_SECTOR_SIZE - 1));
  for (p = file->data; p->length && len > 0; p++)
    {
      if (sector < p->length)
	{
	  grub_size_t size;

	  size = len;
	  if (((size + offset + GRUB_DISK_SECTOR_SIZE - 1)
	       >> GRUB_DISK_SECTOR_BITS) > p->length - sector)
	    size = ((p->length - sector) << GRUB_DISK_SECTOR_BITS) - offset;

	  if (grub_disk_read (file->device->disk, p->offset + sector, offset,
			      size, buf) != GRUB_ERR_NONE)
	    return -1;

	  ret += size;
	  len -= size;
	  sector -= ((size + offset) >> GRUB_DISK_SECTOR_BITS);
	  offset = ((size + offset) & (GRUB_DISK_SECTOR_SIZE - 1));
	}
      else
	sector -= p->length;
    }

  return ret;
}

struct grub_fs grub_fs_blocklist =
  {
    .name = "blocklist",
    .dir = 0,
    .open = grub_fs_blocklist_open,
    .read = grub_fs_blocklist_read,
    .close = 0,
    .next = 0
  };
