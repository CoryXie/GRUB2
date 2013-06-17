/* device.c - device manager */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2005,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/device.h>
#include <grub/disk.h>
#include <grub/net.h>
#include <grub/fs.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/partition.h>
#include <grub/i18n.h>

grub_net_t (*grub_net_open) (const char *name) = NULL;
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
* @brief 打开设备。
*
* @note 注释详细内容:
*
* 本函数实现打开设备的功能。
*
* 如果设备名为空，那么先调用grub_env_get()获取根设备名称；接着尝试使用grub_disk_open()
* 来打开磁盘设备；如果打开失败，那么在尝试使用grub_net_open()来打开网络设备。不论是哪类
* 设备，最后都将打开的设备的句柄保存下来到返回。
**/
grub_device_t
grub_device_open (const char *name)
{
  grub_device_t dev = 0;

  if (! name)
    {
      name = grub_env_get ("root");
      if (name == NULL || *name == '\0')
	{
	  grub_error (GRUB_ERR_BAD_DEVICE,  N_("variable `%s' isn't set"), "root");
	  goto fail;
	}
    }

  dev = grub_malloc (sizeof (*dev));
  if (! dev)
    goto fail;

  dev->net = NULL;
  /* Try to open a disk.  */
  dev->disk = grub_disk_open (name);
  if (dev->disk)
    return dev;
  if (grub_net_open && grub_errno == GRUB_ERR_UNKNOWN_DEVICE)
    {
      grub_errno = GRUB_ERR_NONE;
      dev->net = grub_net_open (name);
    }

  if (dev->net)
    return dev;

 fail:
  grub_free (dev);

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
* @brief 关闭设备。
*
* @note 注释详细内容:
*
* 本函数实现关闭设备的功能。
*
* 如果打开该设备时是打开的磁盘设备，则调用grub_disk_close()；否则如果打开时是网络设备，
* 那么就释放网络设备的资源。
**/
grub_err_t
grub_device_close (grub_device_t device)
{
  if (device->disk)
    grub_disk_close (device->disk);

  if (device->net)
    {
      grub_free (device->net->server);
      grub_free (device->net);
    }

  grub_free (device);

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
* @brief 枚举设备列表，对每个磁盘设备的每个设备分区都调用参数指定的钩子函数。
*
* @note 注释详细内容:
*
* 本函数实现枚举设备列表，对每个设备都调用参数指定的钩子函数的功能。
*
* 使用递归的办法，调用grub_disk_dev_iterate()时指定的函数指针iterate_disk()在被调用
* 时，表示每个被枚举到的磁盘都会被调用一遍；而iterate_disk()又用grub_partition_iterate()
* 来递归调用iterate_partition()，进而可以实现枚举每个磁盘设备的每个设备分区的功能。
**/
int
grub_device_iterate (int (*hook) (const char *name))
{
  auto int iterate_disk (const char *disk_name);
  auto int iterate_partition (grub_disk_t disk,
			      const grub_partition_t partition);

  struct part_ent
  {
    struct part_ent *next;
    char *name;
  } *ents;

  int iterate_disk (const char *disk_name)
    {
      grub_device_t dev;

      if (hook (disk_name))
	return 1;

      dev = grub_device_open (disk_name);
      if (! dev)
	{
	  grub_errno = GRUB_ERR_NONE;
	  return 0;
	}

      if (dev->disk)
	{
	  struct part_ent *p;
	  int ret = 0;

	  ents = NULL;
	  (void) grub_partition_iterate (dev->disk, iterate_partition);
	  grub_device_close (dev);

	  grub_errno = GRUB_ERR_NONE;

	  p = ents;
	  while (p != NULL)
	    {
	      struct part_ent *next = p->next;

	      if (!ret)
		ret = hook (p->name);
	      grub_free (p->name);
	      grub_free (p);
	      p = next;
	    }

	  return ret;
	}

      grub_device_close (dev);
      return 0;
    }

  int iterate_partition (grub_disk_t disk, const grub_partition_t partition)
    {
      struct part_ent *p;
      char *part_name;

      p = grub_malloc (sizeof (*p));
      if (!p)
	{
	  return 1;
	}

      part_name = grub_partition_get_name (partition);
      if (!part_name)
	{
	  grub_free (p);
	  return 1;
	}
      p->name = grub_xasprintf ("%s,%s", disk->name, part_name);
      grub_free (part_name);
      if (!p->name)
	{
	  grub_free (p);
	  return 1;
	}

      p->next = ents;
      ents = p;

      return 0;
    }

  /* Only disk devices are supported at the moment.  */
  return grub_disk_dev_iterate (iterate_disk);
}
