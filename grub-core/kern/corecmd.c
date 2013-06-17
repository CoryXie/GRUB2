/* corecmd.c - critical commands which are registered in kernel */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/mm.h>
#include <grub/dl.h>
#include <grub/err.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/term.h>
#include <grub/file.h>
#include <grub/device.h>
#include <grub/command.h>
#include <grub/i18n.h>

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
* @brief 设置环境变量命令处理函数。
*
* @note 注释详细内容:
*
* 本函数实现设置环境变量命令处理功能。命令格式如下：
*
* set ENVVAR=VALUE
*
* 先将参数使用grub_strchr()，获得由“=”分开的ENVVAR=VALUE类型的环境变量名称以及环境变量
* 的值，然后调用grub_env_set()来实际存储环境变量（名称和值对）。如果参数个数为0，即只
* 调用set，那么就使用grub_env_iterate()循环列出所有的环境变量。
**/
/* set ENVVAR=VALUE */
static grub_err_t
grub_core_cmd_set (struct grub_command *cmd __attribute__ ((unused)),
		   int argc, char *argv[])
{
  char *var;
  char *val;

  auto int print_env (struct grub_env_var *env);

  int print_env (struct grub_env_var *env)
    {
      grub_printf ("%s=%s\n", env->name, env->value);
      return 0;
    }

  if (argc < 1)
    {
      grub_env_iterate (print_env);
      return 0;
    }

  var = argv[0];
  val = grub_strchr (var, '=');
  if (! val)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "not an assignment");

  val[0] = 0;
  grub_env_set (var, val + 1);
  val[0] = '=';

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
* @brief 取消环境变量命令处理函数。
*
* @note 注释详细内容:
*
* 本函数实现取消环境变量命令处理功能。命令格式如下：
*
* unset ENVVAR
*
* 实际是直接调用grub_env_unset()来实际取消环境变量（名称和值对）。
**/
static grub_err_t
grub_core_cmd_unset (struct grub_command *cmd __attribute__ ((unused)),
		     int argc, char *argv[])
{
  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
		       N_("one argument expected"));

  grub_env_unset (argv[0]);
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
* @brief 插入模块命令处理。
*
* @note 注释详细内容:
*
* 本函数实现插入模块命令处理的功能。命令格式如下：
*
* insmod MODULE
*
* 如果参数argv的第一个字节是'/'，'('，或者'+'，那么调用grub_dl_load_file()，否则就调用
* grub_dl_load()来加载对应的模块，并使用grub_dl_ref()来对模块进行一次引用。
**/
/* insmod MODULE */
static grub_err_t
grub_core_cmd_insmod (struct grub_command *cmd __attribute__ ((unused)),
		      int argc, char *argv[])
{
  grub_dl_t mod;

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("one argument expected"));

  if (argv[0][0] == '/' || argv[0][0] == '(' || argv[0][0] == '+')
    mod = grub_dl_load_file (argv[0]);
  else
    mod = grub_dl_load (argv[0]);

  if (mod)
    grub_dl_ref (mod);

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
* @brief 打印设备名称。
*
* @note 注释详细内容:
*
* 本函数实现打印设备名称的功能。
**/
static int
grub_mini_print_devices (const char *name)
{
  grub_printf ("(%s) ", name);

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
* @brief 打印文件名称。
*
* @note 注释详细内容:
*
* 本函数实现打印文件名称的功能。
**/
static int
grub_mini_print_files (const char *filename,
		       const struct grub_dirhook_info *info)
{
  grub_printf ("%s%s ", filename, info->dir ? "/" : "");

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
* @brief 列出文件命令的处理函数。
*
* @note 注释详细内容:
*
* 本函数实现列出文件命令的处理的功能。
*
* 根据参数的路径名使用grub_file_get_device_name()找到存储设备名，然后根据设备名
* 使用grub_device_open()来打开设备，并使用grub_fs_probe()探测设备使用的是何种文
* 件系统,进而使用对应的文件系统的dir函数指针来实现列出文件的功能。之后关闭设备。
**/
/* ls [ARG] */
static grub_err_t
grub_core_cmd_ls (struct grub_command *cmd __attribute__ ((unused)),
		  int argc, char *argv[])
{
  if (argc < 1)
    {
      grub_device_iterate (grub_mini_print_devices);grub_file_get_device_name
      grub_xputs ("\n");
      grub_refresh ();
    }
  else
    {
      char *device_name;
      grub_device_t dev = 0;
      grub_fs_t fs;
      char *path;

      device_name = grub_file_get_device_name (argv[0]);
      if (grub_errno)
	goto fail;
      dev = grub_device_open (device_name);
      if (! dev)
	goto fail;

      fs = grub_fs_probe (dev);
      path = grub_strchr (argv[0], ')');
      if (! path)
	path = argv[0];
      else
	path++;

      if (! path && ! device_name)
	{
	  grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid argument");
	  goto fail;
	}

      if (! path)
	{
	  if (grub_errno == GRUB_ERR_UNKNOWN_FS)
	    grub_errno = GRUB_ERR_NONE;

	  grub_printf ("(%s): Filesystem is %s.\n",
		       device_name, fs ? fs->name : "unknown");
	}
      else if (fs)
	{
	  (fs->dir) (dev, path, grub_mini_print_files);
	  grub_xputs ("\n");
	  grub_refresh ();
	}

    fail:
      if (dev)
	grub_device_close (dev);

      grub_free (device_name);
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
* @brief 注册核心（在rescue模式下可用的）命令。
*
* @note 注释详细内容:
*
* 本函数实现注册核心（在rescue模式下可用的）命令的功能。
*
* 包括set，unset，ls，以及insmod这几个关键的核心命令。有了这几个命令，就可以处理
* 环境变量，列出文件，以及加载模块等基本功能。
**/
void
grub_register_core_commands (void)
{
  grub_command_t cmd;
  cmd = grub_register_command ("set", grub_core_cmd_set,
			       N_("[ENVVAR=VALUE]"),
			       N_("Set an environment variable."));
  if (cmd)
    cmd->flags |= GRUB_COMMAND_FLAG_EXTRACTOR;
  grub_register_command ("unset", grub_core_cmd_unset,
			 N_("ENVVAR"),
			 N_("Remove an environment variable."));
  grub_register_command ("ls", grub_core_cmd_ls,
			 N_("[ARG]"), N_("List devices or files."));
  grub_register_command ("insmod", grub_core_cmd_insmod,
			 N_("MODULE"), N_("Insert a module."));
}
