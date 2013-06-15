/* main.c - the kernel main routine */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2006,2008,2009  Free Software Foundation, Inc.
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

#include <grub/kernel.h>
#include <grub/misc.h>
#include <grub/symbol.h>
#include <grub/dl.h>
#include <grub/term.h>
#include <grub/file.h>
#include <grub/device.h>
#include <grub/env.h>
#include <grub/mm.h>
#include <grub/command.h>
#include <grub/reader.h>
#include <grub/parser.h>

/* This is actualy platform-independant but used only on loongson and sparc.  */
#if defined (GRUB_MACHINE_MIPS_LOONGSON) || defined (GRUB_MACHINE_MIPS_QEMU_MIPS) || defined (GRUB_MACHINE_SPARC64)
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
* @brief 获得模块数组的结束地址。
*
* @note 注释详细内容: 
*
* 本函数实现获取模块数组的结束地址的功能。如果grub_modbase还未被初始化，则
* 直接返回grub_modbase本身(表示没有模块在数组中，结束地址与基地址相同)；否
* 则将grub_modbase强制转换为grub_module_info结构modinfo，返回grub_modbase + 
* modinfo->size (说明在grub_modbase处存放的是grub_module_info结构，每次添加
* 模块时会更改modinfo->size来反应实际的模块数组的大小)。
**/
grub_addr_t
grub_modules_get_end (void)
{
  struct grub_module_info *modinfo;

  modinfo = (struct grub_module_info *) grub_modbase;

  /* Check if there are any modules.  */
  if ((modinfo == 0) || modinfo->magic != GRUB_MODULE_MAGIC)
    return grub_modbase;

  return grub_modbase + modinfo->size;
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
* @brief 加载GRUB2核心的所有ELF模块。
*
* @note 注释详细内容: 
*
* 本函数实现加载GRUB2核心的所有ELF模块的功能。调用FOR_MODULES，对每个模块,如果
* 该模块是OBJ_TYPE_ELF类型的，就调用grub_dl_load_core()来实际加载，如果出错就打
* 印错误消息。
**/

/* Load all modules in core.  */
static void
grub_load_modules (void)
{
  struct grub_module_header *header;
  FOR_MODULES (header)
  {
    /* Not an ELF module, skip.  */
    if (header->type != OBJ_TYPE_ELF)
      continue;

    if (! grub_dl_load_core ((char *) header + sizeof (struct grub_module_header),
			     (header->size - sizeof (struct grub_module_header))))
      grub_fatal ("%s", grub_errmsg);

    if (grub_errno)
      grub_print_error ();
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
* @brief 加载并执行GRUB2核心的配置模块。
*
* @note 注释详细内容: 
*
* 本函数实现加载GRUB2核心的所有配置模块的功能。调用FOR_MODULES，找到第一个类型
* 是OBJ_TYPE_CONFIG类型的模块，并调用grub_parser_execute()来实际解析配置并执行
* 里面的命令，然后退出。
**/
static void
grub_load_config (void)
{
  struct grub_module_header *header;
  FOR_MODULES (header)
  {
    /* Not an embedded config, skip.  */
    if (header->type != OBJ_TYPE_CONFIG)
      continue;
    
    grub_parser_execute ((char *) header +
			 sizeof (struct grub_module_header));
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
* @brief 删除包绕参数的括号(如果存在的话)。
*
* @note 注释详细内容: 
*
* 本函数实现删除包绕参数val两边的括号的功能。该函数作为根环境变量的写钩子
* (Write hook)。
**/

/* Write hook for the environment variables of root. Remove surrounding
   parentheses, if any.  */
static char *
grub_env_write_root (struct grub_env_var *var __attribute__ ((unused)),
		     const char *val)
{
  /* XXX Is it better to check the existence of the device?  */
  grub_size_t len = grub_strlen (val);

  if (val[0] == '(' && val[len - 1] == ')')
    return grub_strndup (val + 1, len - 2);

  return grub_strdup (val);
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
* @brief 设置GRUB2的prefix和root环境变量。
*
* @note 注释详细内容: 
*
* 本函数实现设置GRUB2的prefix和root环境变量的功能。
**/

static void
grub_set_prefix_and_root (void)
{
  char *device = NULL;
  char *path = NULL;
  char *fwdevice = NULL;
  char *fwpath = NULL;
  char *prefix = NULL;
  struct grub_module_header *header;

  FOR_MODULES (header)
    if (header->type == OBJ_TYPE_PREFIX)
      prefix = (char *) header + sizeof (struct grub_module_header);

  grub_register_variable_hook ("root", 0, grub_env_write_root);

  if (prefix)
    {
      char *pptr = NULL;
      if (prefix[0] == '(')
	{
	  pptr = grub_strrchr (prefix, ')');
	  if (pptr)
	    {
	      device = grub_strndup (prefix + 1, pptr - prefix - 1);
	      pptr++;
	    }
	}
      if (!pptr)
	pptr = prefix;
      if (pptr[0])
	path = grub_strdup (pptr);
    }
  if ((!device || device[0] == ',' || !device[0]) || !path)
    grub_machine_get_bootlocation (&fwdevice, &fwpath);

  if (!device && fwdevice)
    device = fwdevice;
  else if (fwdevice && (device[0] == ',' || !device[0]))
    {
      /* We have a partition, but still need to fill in the drive.  */
      char *comma, *new_device;

      for (comma = fwdevice; *comma; )
	{
	  if (comma[0] == '\\' && comma[1] == ',')
	    {
	      comma += 2;
	      continue;
	    }
	  if (*comma == ',')
	    break;
	  comma++;
	}
      if (*comma)
	{
	  char *drive = grub_strndup (fwdevice, comma - fwdevice);
	  new_device = grub_xasprintf ("%s%s", drive, device);
	  grub_free (drive);
	}
      else
	new_device = grub_xasprintf ("%s%s", fwdevice, device);

      grub_free (fwdevice);
      grub_free (device);
      device = new_device;
    }
  else
    grub_free (fwdevice);
  if (fwpath && !path)
    path = fwpath;
  else
    grub_free (fwpath);
  if (device)
    {
      char *prefix_set;
    
      prefix_set = grub_xasprintf ("(%s)%s", device, path ? : "");
      if (prefix_set)
	{
	  grub_env_set ("prefix", prefix_set);
	  grub_free (prefix_set);
	}
      grub_env_set ("root", device);
    }

  grub_free (device);
  grub_free (path);
  grub_print_error ();
}

/* Load the normal mode module and execute the normal mode if possible.  */
static void
grub_load_normal_mode (void)
{
  /* Load the module.  */
  grub_dl_load ("normal");

  /* Print errors if any.  */
  grub_print_error ();
  grub_errno = 0;

  grub_command_execute ("normal", 0, 0);
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
* @brief GRUB2的主函数入口。
*
* @note 注释详细内容:
*
* grub_main()是GRUB2的核心工作循环，经过对该核心循环的分析，可以得出GRUB的核心
* 软件架构。
* 
* 1) 注册导出符号
*    
*    grub_register_exported_symbols() 函数通过gensymlist.sh脚本在预处理阶段扫描
*    各个头文件，找出被标为EXPORT_FUNC或者EXPORT_VAR的函数或者变量，生成一个导
*    出的符号表数组。然后对该数组的每项都调用grub_dl_register_symbol()进行注册。
*    grub_dl_register_symbol()通过对符号名字进行hash处理，将该符号映射到
*    grub_symtab[]数组的对应项中。
*
* 2) 加载核心模块
* 
*    当在初始化过程对第三阶段解压缩过程中，实际上已经将GRUB kernel和各模块都解
*    压缩到GRUB_MEMORY_MACHINE_DECOMPRESSION_ADDR 开始处，也就是0x100000处，而
*    kernel部分已经被复制回其链接地址0x9000处，但是模块部分还在原来被解压缩的
*    地方。grub_load_modules() 函数就是从模块部分加载各模块。该部分主要是通过
*    FOR_MODULES宏扫描所有的模块头，并通过grub_dl_load_core()函数来实现模块的实
*    际加载。加载之后会调用模块的初始化函数，并将模块连接进入链表。
*
* 3) 注册核心命令
*
*    grub_register_core_commands()函数grub_register_command()分别注册如下几个核
*    心命令: 
*
*    - 命令	        功能	            处理函数
*
*    - set	        设置环境变量值	    grub_core_cmd_set()
*    - unset	    删除环境变量	    grub_core_cmd_unset()
*    - ls	        列出设备或者文件	grub_core_cmd_ls()
*    - insmod	    插入模块	        grub_core_cmd_insmod()
*
*    grub_register_command()实际上调用的是grub_register_command_prio()，将命令
*    加入到grub_command_list列表中。
*
* 4) 加载内嵌配置
*
*    grub_load_config()函数对模块组内每个属于OBJ_TYPE_CONFIG类型的模块调用函数
*    grub_parser_execute()，扫描该配置模块内的每一行，解析改行，找到对应命令，
*    并执行相关命令。这些配置模块是嵌入到模块组内的对GRUB的初始化配置。
*
* 5) 执行常规模式
*
*    grub_load_normal_mode()通过调用grub_dl_load()加载名为"normal"的模块，并通
*    过grub_command_execute()运行该模块。
*
*    其中，grub_dl_load()函数首先通过grub_dl_get()查看是否有对应名字的模块已经
*    被加载到了以grub_dl_head为首的模块链表中，如果有的话，则直接返回该模块。
*    否则，grub_dl_load()函数就在GRUB的安装目录的lib目录(grub_dl_dir)下对应
*    CPU平台（GRUB_TARGET_CPU以及GRUB_PLATFORM）目录下，寻找对应名字的*.mod模
*    块，并通过grub_dl_load_file()，进而调用grub_dl_load_core()将模块加载进入
*    grub_dl_head为首的模块链表中。
*
*    而grub_command_execute()则是通过grub_command_find()在grub_command_list列
*    表上查找对应命令，若找到命令，则执行之。
*
*    因此，对于这里的名为"normal"的模块的执行，由于之前并没有载入过该模块，因
*    此必定是先从CPU平台（例如lib/i386-pc/）下面去寻找normal.mod，然后加载进入
*    grub_dl_head链表，并执行该模块的初始化函数。"normal"模块是GRUB的第一个执行
*    的模块，其他的模块都可以在它的引导下被进一步载入执行。"normal"模块源代码位
*    于grub-core/normal目录下，观察该目录下的文件，可以发现有grub-core/normal/main.c，
*    对于"normal"模块，其初始化函数就是该文件中的GRUB_MOD_INIT(normal)所对应的
*    函数。
*
*    GRUB_MOD_INIT(normal)所对应的函数会执行一些关键动作，例如调用grub_dl_load()
*    加载公用的"gzio"模块，调用一些子函数注册下列关键命令：
*
*    - 命令	            功能	                        处理函数
*
*    - authenticate	    检查用户是否在USERLIST之中	    grub_cmd_authenticate ()
*    - export	        导出变量	                    grub_cmd_export ()
*    - break	        跳出循环	                    grub_script_break ()
*    - continue	        继续循环	                    grub_script_break ()
*    - shift	        移动定位参数（$0, $1, $2, ...）	grub_script_shift()
*    - setparams	    设置定位参数	                grub_script_setparams()
*    - return	        从一个函数返回(与bash语义一致)  grub_script_return()
*    - menuentry	    定义一个菜单项	                grub_cmd_menuentry()
*    - submenu	        定义一个子菜单	                grub_cmd_menuentry()
*    - clear	        清屏幕	                        grub_mini_cmd_clear()
*    - normal	        进入normal模式	                grub_cmd_normal()
*    - normal_exit	    退出normal模式	                grub_cmd_normal_exit()
*
*    由于在normal模块的初始化过程中已经注册了normal命令，因此在grub_command_execute()
*    时就能在grub_command_list列表上查找到该命令，从而执行grub_cmd_normal()函数，
*    在这个函数中，尝试找到一个配置文件，例如grub.cfg，之后调用函数
*    grub_enter_normal_mode()，进入GRUB的常规模式。
*
*    grub_cmd_normal()函数会尝试读入并执行配置文件(例如安装目录下的grub.cfg)。
*    这里是用户正常启动时常用的配置方式，例如可以指定要启动的操作系统的菜单，可
*    以选择启动哪一个。
*
*    接着进入grub_cmdline_run()，这是一个while (1)循环，读取用户输入，解释并执行
*   （通过grub_script_execute()）。该循环直到用户输入normal_exit命令才通过
*    grub_cmd_normal_exit()结束。
*
* 6) 进入救援模式
*
*    如果前一步的"normal"的模块执行退出，则GRUB进入救援模式(rescue mode)，即执
*    行函数grub_rescue_run()。该函数式个while(1)循环函数，不断读取用户命令行输
*    入，解析命令，并执行之。其中对用户输入的命令解析都是通过grub_command_find()
*    完成的。该函数永远不会返回，因此GRUB也就不会返回。
**/

/* The main routine.  */
void __attribute__ ((noreturn))
grub_main (void)
{
  /* First of all, initialize the machine.  */
  grub_machine_init ();

  /* Hello.  */
  grub_setcolorstate (GRUB_TERM_COLOR_HIGHLIGHT);
  grub_printf ("Welcome to GRUB!\n\n");
  grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);

  /* Load pre-loaded modules and free the space.  */
  grub_register_exported_symbols ();
#ifdef GRUB_LINKER_HAVE_INIT
  grub_arch_dl_init_linker ();
#endif  
  grub_load_modules ();

  /* It is better to set the root device as soon as possible,
     for convenience.  */
  grub_set_prefix_and_root ();
  grub_env_export ("root");
  grub_env_export ("prefix");

  grub_register_core_commands ();

  grub_load_config ();
  grub_load_normal_mode ();
  grub_rescue_run ();
}
