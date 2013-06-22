/* file.c - file I/O functions */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2006,2007,2009  Free Software Foundation, Inc.
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
#include <grub/err.h>
#include <grub/file.h>
#include <grub/net.h>
#include <grub/mm.h>
#include <grub/fs.h>
#include <grub/device.h>
#include <grub/i18n.h>

void (*EXPORT_VAR (grub_grubnet_fini)) (void);

grub_file_filter_t grub_file_filters_all[GRUB_FILE_FILTER_MAX];
grub_file_filter_t grub_file_filters_enabled[GRUB_FILE_FILTER_MAX];

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
* @brief 获取文件名参数name的中的设备名部分。
*
* @note 注释详细内容:
*
* 本函数实现获取文件名参数name的中的设备名部分的功能。文件名由设备名加上实际的文件
* 路径名组成，其中设备名的格式是"(hd0)“，因此本函数就是要返回"(hd0)“中的"hd0“部分。
**/
/* Get the device part of the filename NAME. It is enclosed by parentheses.  */
char *
grub_file_get_device_name (const char *name)
{
  if (name[0] == '(')
    {
      char *p = grub_strchr (name, ')');
      char *ret;

      if (! p)
	{
	  grub_error (GRUB_ERR_BAD_FILENAME, N_("missing `%c' symbol"), ')');
	  return 0;
	}

      ret = (char *) grub_malloc (p - name);
      if (! ret)
	return 0;

      grub_memcpy (ret, name + 1, p - name - 1);
      ret[p - name - 1] = '\0';
      return ret;
    }

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
* @brief 打开文件。
*
* @note 注释详细内容:
*
* 本函数实现打开文件的功能。首先使用grub_file_get_device_name()获得文件名的设备部分；然后
* 获得文件名的文件部分；然后再调用grub_device_open()打开设备；然后分配一个grub_file_t结构，
* 如果文件名部分不是以'/'开始的话，则使用grub_fs_blocklist以block list的方式读取文件；否则
* grub_fs_probe()探测实际的文件系统；之后使用file->fs->open()来打开文件；然后对所有的使用
* grub_file_filters_enabled[]使能的文件过滤器，都调用该函数指针。再用grub_file_filters_all
* 来拷贝给grub_file_filters_enabled[]。
**/
grub_file_t
grub_file_open (const char *name)
{
  grub_device_t device = 0;
  grub_file_t file = 0, last_file = 0;
  char *device_name;
  char *file_name;
  grub_file_filter_id_t filter;

  device_name = grub_file_get_device_name (name);
  if (grub_errno)
    goto fail;

  /* Get the file part of NAME.  */
  file_name = (name[0] == '(') ? grub_strchr (name, ')') : NULL;
  if (file_name)
    file_name++;
  else
    file_name = (char *) name;

  device = grub_device_open (device_name);
  grub_free (device_name);
  if (! device)
    goto fail;

  file = (grub_file_t) grub_zalloc (sizeof (*file));
  if (! file)
    goto fail;

  file->device = device;

  if (device->disk && file_name[0] != '/')
    /* This is a block list.  */
    file->fs = &grub_fs_blocklist;
  else
    {
      file->fs = grub_fs_probe (device);
      if (! file->fs)
	goto fail;
    }

  if ((file->fs->open) (file, file_name) != GRUB_ERR_NONE)
    goto fail;

  for (filter = 0; file && filter < ARRAY_SIZE (grub_file_filters_enabled);
       filter++)
    if (grub_file_filters_enabled[filter])
      {
	last_file = file;
	file = grub_file_filters_enabled[filter] (file);
      }
  if (!file)
    grub_file_close (last_file);

  grub_memcpy (grub_file_filters_enabled, grub_file_filters_all,
	       sizeof (grub_file_filters_enabled));

  return file;

 fail:
  if (device)
    grub_device_close (device);

  /* if (net) grub_net_close (net);  */

  grub_free (file);

  grub_memcpy (grub_file_filters_enabled, grub_file_filters_all,
	       sizeof (grub_file_filters_enabled));

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
* @brief 读取文件内容到缓冲区。
*
* @note 注释详细内容:
*
* 本函数实现读取文件内容到缓冲区的功能。首先进行一系列的参数检查，例如偏移量不能超过文件
* 大小，读取大小不能为0，也不能超过文件剩余的大小等等，然后调用文件系统驱动的read()接口
* 函数来实际读取文件内容。
**/
grub_ssize_t
grub_file_read (grub_file_t file, void *buf, grub_size_t len)
{
  grub_ssize_t res;

  if (file->offset > file->size)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE,
		  N_("attempt to read past the end of file"));
      return -1;
    }

  if (len == 0)
    return 0;

  if (len > file->size - file->offset)
    len = file->size - file->offset;

  /* Prevent an overflow.  */
  if ((grub_ssize_t) len < 0)
    len >>= 1;

  if (len == 0)
    return 0;
  res = (file->fs->read) (file, buf, len);
  if (res > 0)
    file->offset += res;

  return res;
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
* @brief 关闭文件。
*
* @note 注释详细内容:
*
* 本函数实现关闭文件的功能。首先调用文件系统驱动的close()函数指针关闭文件；然后如果是
* 实际的磁盘设备，则还要调用grub_device_close()关闭磁盘设备；最后释放文件结构。
**/
grub_err_t
grub_file_close (grub_file_t file)
{
  if (file->fs->close)
    (file->fs->close) (file);

  if (file->device)
    grub_device_close (file->device);
  grub_free (file);
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
* @brief 定位文件位置。
*
* @note 注释详细内容:
*
* 本函数实现定位文件位置的功能。将当前的文件偏移量保存为old，然后更新当前的偏移量为参
* 数指定值，最后返回保存的old偏移量。
**/
grub_off_t
grub_file_seek (grub_file_t file, grub_off_t offset)
{
  grub_off_t old;

  if (offset > file->size)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE,
		  N_("attempt to seek outside of the file"));
      return -1;
    }

  old = file->offset;
  file->offset = offset;

  return old;
}
