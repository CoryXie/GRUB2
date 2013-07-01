/* main.c - the normal mode main routine */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000,2001,2002,2003,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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
#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/term.h>
#include <grub/env.h>
#include <grub/parser.h>
#include <grub/reader.h>
#include <grub/menu_viewer.h>
#include <grub/auth.h>
#include <grub/i18n.h>
#include <grub/charset.h>
#include <grub/script_sh.h>

GRUB_MOD_LICENSE ("GPLv3+");

#define GRUB_DEFAULT_HISTORY_SIZE	50

static int nested_level = 0;
int grub_normal_exit_level = 0;

/* Read a line from the file FILE.  */
char *
grub_file_getline (grub_file_t file)
{
  char c;
  grub_size_t pos = 0;
  char *cmdline;
  int have_newline = 0;
  grub_size_t max_len = 64;

  /* Initially locate some space.  */
  cmdline = grub_malloc (max_len);
  if (! cmdline)
    return 0;

  while (1)
    {
      if (grub_file_read (file, &c, 1) != 1)
	break;

      /* Skip all carriage returns.  */
      if (c == '\r')
	continue;


      if (pos + 1 >= max_len)
	{
	  char *old_cmdline = cmdline;
	  max_len = max_len * 2;
	  cmdline = grub_realloc (cmdline, max_len);
	  if (! cmdline)
	    {
	      grub_free (old_cmdline);
	      return 0;
	    }
	}

      if (c == '\n')
	{
	  have_newline = 1;
	  break;
	}

      cmdline[pos++] = c;
    }

  cmdline[pos] = '\0';

  /* If the buffer is empty, don't return anything at all.  */
  if (pos == 0 && !have_newline)
    {
      grub_free (cmdline);
      cmdline = 0;
    }

  return cmdline;
}

void
grub_normal_free_menu (grub_menu_t menu)
{
  grub_menu_entry_t entry = menu->entry_list;

  while (entry)
    {
      grub_menu_entry_t next_entry = entry->next;
      grub_size_t i;

      if (entry->classes)
	{
	  struct grub_menu_entry_class *class;
	  for (class = entry->classes; class; class = class->next)
	    grub_free (class->name);
	  grub_free (entry->classes);
	}

      if (entry->args)
	{
	  for (i = 0; entry->args[i]; i++)
	    grub_free (entry->args[i]);
	  grub_free (entry->args);
	}

      grub_free ((void *) entry->id);
      grub_free ((void *) entry->users);
      grub_free ((void *) entry->title);
      grub_free ((void *) entry->sourcecode);
      entry = next_entry;
    }

  grub_free (menu);
  grub_env_unset_menu ();
}

static grub_menu_t
read_config_file (const char *config)
{
  grub_file_t file;
  const char *old_file, *old_dir;
  char *config_dir, *ptr = 0;

  auto grub_err_t getline (char **line, int cont);
  grub_err_t getline (char **line, int cont __attribute__ ((unused)))
    {
      while (1)
	{
	  char *buf;

	  *line = buf = grub_file_getline (file);
	  if (! buf)
	    return grub_errno;

	  if (buf[0] == '#')
	    grub_free (*line);
	  else
	    break;
	}

      return GRUB_ERR_NONE;
    }

  grub_menu_t newmenu;

  newmenu = grub_env_get_menu ();
  if (! newmenu)
    {
      newmenu = grub_zalloc (sizeof (*newmenu));
      if (! newmenu)
	return 0;

      grub_env_set_menu (newmenu);
    }

  /* Try to open the config file.  */
  file = grub_file_open (config);
  if (! file)
    return 0;

  old_file = grub_env_get ("config_file");
  old_dir = grub_env_get ("config_directory");
  grub_env_set ("config_file", config);
  config_dir = grub_strdup (config);
  if (config_dir)
    ptr = grub_strrchr (config_dir, '/');
  if (ptr)
    *ptr = 0;
  grub_env_set ("config_directory", config_dir);

  grub_env_export ("config_file");
  grub_env_export ("config_directory");

  while (1)
    {
      char *line;

      /* Print an error, if any.  */
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;

      if ((getline (&line, 0)) || (! line))
	break;

      grub_normal_parse_line (line, getline);
      grub_free (line);
    }

  if (old_file)
    grub_env_set ("config_file", old_file);
  else
    grub_env_unset ("config_file");
  if (old_dir)
    grub_env_set ("config_directory", old_dir);
  else
    grub_env_unset ("config_directory");

  grub_file_close (file);

  return newmenu;
}

/* Initialize the screen.  */
void
grub_normal_init_page (struct grub_term_output *term)
{
  grub_ssize_t msg_len;
  int posx;
  const char *msg = _("GNU GRUB  version %s");
  char *msg_formatted;
  grub_uint32_t *unicode_msg;
  grub_uint32_t *last_position;
 
  grub_term_cls (term);

  msg_formatted = grub_xasprintf (msg, PACKAGE_VERSION);
  if (!msg_formatted)
    return;
 
  msg_len = grub_utf8_to_ucs4_alloc (msg_formatted,
  				     &unicode_msg, &last_position);
  grub_free (msg_formatted);
 
  if (msg_len < 0)
    {
      return;
    }

  posx = grub_getstringwidth (unicode_msg, last_position, term);
  posx = (grub_term_width (term) - posx) / 2;
  grub_term_gotoxy (term, posx, 1);

  grub_print_ucs4 (unicode_msg, last_position, 0, 0, term);
  grub_putcode ('\n', term);
  grub_putcode ('\n', term);
  grub_free (unicode_msg);
}

static void
read_lists (const char *val)
{
  if (! grub_no_autoload)
    {
      read_command_list (val);
      read_fs_list (val);
      read_crypto_list (val);
      read_terminal_list (val);
    }
  grub_gettext_reread_prefix (val);
}

static char *
read_lists_hook (struct grub_env_var *var __attribute__ ((unused)),
		 const char *val)
{
  read_lists (val);
  return val ? grub_strdup (val) : NULL;
}

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月28日
*
* @brief 执行normal命令。
*
* @note 注释详细内容:
*
* 当grub_load_normal_mode()函数加载完常规模块并调用其初始化函数之后，会调用
* grub_command_execute()来执行常规模式命令进而进入GRUB2的常规模式。函数grub_command_execute()
* 位于【grub-2.00/include/grub/command.h】。
*
* 该函数grub_command_execute()函数是通过grub_command_find()在grub_command_list列表上查找对应
* 命令，若找到命令，则执行之。
*
* 由于在normal模块GRUB_MOD_INIT(normal)的初始化过程中已经注册了normal命令，因此在
* grub_command_execute()时就能在grub_command_list列表上查找到该命令，从而执行位于
* 【grub-2.00/grub-core/normal/main.c】的grub_cmd_normal()函数。
* 
* 在这个函数中，将会尝试从$prefix环境变量指定的路径找到一个配置文件，例如grub.cfg，之后调用函
* 数grub_enter_normal_mode()，进入GRUB的常规模式。
* 
* 函数grub_cmd_normal()会调用【grub-2.00/grub-core/normal/main.c】的grub_enter_normal_mode()函数。
* 而函数grub_enter_normal_mode()尝试调用【grub-2.00/grub-core/normal/main.c】的grub_normal_execute()
* 函数来读入并执行配置文件(该文件通常是/boot/grub/grub.cfg)。这里是用户正常启动时常用的配置方式，
* 例如可以指定要启动的操作系统的菜单，可以选择启动哪一个。
*
* 该grub_normal_execute()函数完成了如下步骤：
*
* 1）	如果不是嵌套调用该函数，则调用grub_env_get()获得环境变量$prefix的值（即grub.cfg
*     和*.mod模块所在的目录）；并使用该$prefix指定的路径来调用read_lists()去读入command.lst，
*     fs.lst，crypto.lst以及terminal.lst等文件的内容；进而注册环境变量$prefix的写钩子函数为
*     read_lists_hook()。
* 2）	如果有配置文件，则调用read_config_file (config)来读入并解析配置文件。解析的结果为用于
*     显示的菜单信息，以grub_menu_t返回。
* 3）	如果解析配置文件得出有菜单需要显示，则调用grub_show_menu()显示菜单。
**/
/* Read the config file CONFIG and execute the menu interface or
   the command line interface if BATCH is false.  */
void
grub_normal_execute (const char *config, int nested, int batch)
{
  grub_menu_t menu = 0;
  const char *prefix;

  if (! nested)
    {
      prefix = grub_env_get ("prefix");
      read_lists (prefix);
      grub_register_variable_hook ("prefix", NULL, read_lists_hook);
    }

  if (config)
    {
      menu = read_config_file (config);

      /* Ignore any error.  */
      grub_errno = GRUB_ERR_NONE;
    }

  if (! batch)
    {
      if (menu && menu->size)
	{
	  grub_show_menu (menu, nested, 0);
	  if (nested)
	    grub_normal_free_menu (menu);
	}
    }
}

/* This starts the normal mode.  */
void
grub_enter_normal_mode (const char *config)
{
  nested_level++;
  grub_normal_execute (config, 0, 0);
  grub_cmdline_run (0);
  nested_level--;
  if (grub_normal_exit_level)
    grub_normal_exit_level--;
}

/* Enter normal mode from rescue mode.  */
static grub_err_t
grub_cmd_normal (struct grub_command *cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  if (argc == 0)
    {
      /* Guess the config filename. It is necessary to make CONFIG static,
	 so that it won't get broken by longjmp.  */
      char *config;
      const char *prefix;

      prefix = grub_env_get ("prefix");
      if (prefix)
	{
	  config = grub_xasprintf ("%s/grub.cfg", prefix);
	  if (! config)
	    goto quit;

	  grub_enter_normal_mode (config);
	  grub_free (config);
	}
      else
	grub_enter_normal_mode (0);
    }
  else
    grub_enter_normal_mode (argv[0]);

quit:
  return 0;
}

/* Exit from normal mode to rescue mode.  */
static grub_err_t
grub_cmd_normal_exit (struct grub_command *cmd __attribute__ ((unused)),
		      int argc __attribute__ ((unused)),
		      char *argv[] __attribute__ ((unused)))
{
  if (nested_level <= grub_normal_exit_level)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "not in normal environment");
  grub_normal_exit_level++;
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_normal_reader_init (int nested)
{
  struct grub_term_output *term;
  const char *msg = _("Minimal BASH-like line editing is supported. For "
		      "the first word, TAB lists possible command completions. Anywhere "
		      "else TAB lists possible device or file completions. %s");
  const char *msg_esc = _("ESC at any time exits.");
  char *msg_formatted;

  msg_formatted = grub_xasprintf (msg, nested ? msg_esc : "");
  if (!msg_formatted)
    return grub_errno;

  FOR_ACTIVE_TERM_OUTPUTS(term)
  {
    grub_normal_init_page (term);
    grub_term_setcursor (term, 1);

    grub_print_message_indented (msg_formatted, 3, STANDARD_MARGIN, term);
    grub_putcode ('\n', term);
    grub_putcode ('\n', term);
  }
  grub_free (msg_formatted);
 
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
* @date 注释添加日期：2013年6月28日
*
* @brief 读入用户命令行输入。
*
* @note 注释详细内容:
*
* 函数首先要输出一个命令行提示符，提示用户输入。本函数的参数cont，表示该函数是不是对前
* 一次读入一行命令输入的继续，如果是，则cont为1，因此输出的提示符就是“>”，否则就输出
* “grub>”作为新行的提示符。之后是一个while(1)循环，使用grub_cmdline_get()来得到用户的
* 一行输入，如果返回非0，则表示得到了一行用户输入，因此返回该输入的命令。
**/
static grub_err_t
grub_normal_read_line_real (char **line, int cont, int nested)
{
  const char *prompt;

  if (cont)
    /* TRANSLATORS: it's command line prompt.  */
    prompt = _(">");
  else
    /* TRANSLATORS: it's command line prompt.  */
    prompt = _("grub>");

  if (!prompt)
    return grub_errno;

  while (1)
    {
      *line = grub_cmdline_get (prompt);
      if (*line)
	return 0;

      if (cont || nested)
	{
	  grub_free (*line);
	  *line = 0;
	  return grub_errno;
	}
    }
 
}

static grub_err_t
grub_normal_read_line (char **line, int cont)
{
  return grub_normal_read_line_real (line, cont, 0);
}

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月28日
*
* @brief 运行命令行。
*
* @note 注释详细内容:
*
* 如果grub_normal_execute()函数返回，则grub_enter_normal_mode()接着进入grub_cmdline_run()。
*
* 进入命令行之前先有调用grub_auth_check_authentication()进行身份认证，认证失败是不能继续
* 运行命令行界面的。接着调用grub_normal_reader_init()初始化终端，并且打印一段描述命令行
* 的消息。再之后就是是一个while (1)循环，调用grub_normal_read_line_real()读取用户输入，
* 使用函数grub_normal_parse_line()解释并执行（通过grub_script_execute()）。该循环直到用
* 户输入normal_exit命令才通过grub_cmd_normal_exit()结束。
**/
void
grub_cmdline_run (int nested)
{
  grub_err_t err = GRUB_ERR_NONE;

  err = grub_auth_check_authentication (NULL);

  if (err)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
      return;
    }

  grub_normal_reader_init (nested);

  while (1)
    {
      char *line;

      if (grub_normal_exit_level)
	break;

      /* Print an error, if any.  */
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;

      grub_normal_read_line_real (&line, 0, nested);
      if (! line)
	break;

      grub_normal_parse_line (line, grub_normal_read_line);
      grub_free (line);
    }
}

static char *
grub_env_write_pager (struct grub_env_var *var __attribute__ ((unused)),
		      const char *val)
{
  grub_set_more ((*val == '1'));
  return grub_strdup (val);
}

/* clear */
static grub_err_t
grub_mini_cmd_clear (struct grub_command *cmd __attribute__ ((unused)),
		   int argc __attribute__ ((unused)),
		   char *argv[] __attribute__ ((unused)))
{
  grub_cls ();
  return 0;
}

static grub_command_t cmd_clear;

static void (*grub_xputs_saved) (const char *str);
static const char *features[] = {
  "feature_chainloader_bpb", "feature_ntldr", "feature_platform_search_hint",
  "feature_default_font_path", "feature_all_video_module",
  "feature_menuentry_id", "feature_menuentry_options", "feature_200_final"
};


/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月28日
*
* @brief 紧凑内存分布描述符数组。
*
* @note 注释详细内容:
*
* 我们知道在grub_dl_load_core()的后面会调用grub_dl_call_init()来调用加载进来的模块的init()
* 函数指针，对该模块进行初始化。因此，对于这里的名为“normal”的模块的执行，由于之前并没有
* 载入过该模块，因此必定是先从CPU平台（例如$prefix/lib/i386-pc/）下面去寻找normal.mod，
* 然后加载进入grub_dl_head链表，并执行该模块的初始化函数。“normal”模块是GRUB的第一个执行
* 的模块，其他的模块都可以在它的引导下被进一步载入执行。对于“normal”模块，其初始化函数就
* 是本文件中的GRUB_MOD_INIT(normal)所对应的函数。
*
* GRUB_MOD_INIT(normal)所对应的函数会执行如下一些关键动作，将 normal 模块的信息加载进入
* GRUB2的核心。模块信息包含用户认证信息、模块目录、模块命令等，最后设定颜色的环境变量。
* 
* 1）	调用grub_dl_load()加载公用的"gzio"模块，用以解压以gzip格式压缩的数据。
* 2）	调用grub_normal_auth_init()函数注册用户认证命令，命令名称为 authenticate，命令的执行
*     函数为【grub-2.00/grub-core/normal/auth.c】 的 grub_cmd_authenticate() 函数。
* 3）	调用grub_context_init()函数注册context 命令，命令名称为 export，指令的执行函数为 
*     【grub-2.00/grub-core/normal/context.c】的 grub_cmd_export() 函数。
* 4）	调用grub_script_init()函数注册 script 命令集，命令名称为 break、continue、shift、
*     setparams、return 等， 这些命令的执行函数为【grub-2.00/grub-core/script/execute.c】的 
*     grub_script_break()、grub_script_shift()、grub_script_setparams()、grub_script_return() 
*     等函数。
* 5）	调用grub_menu_init()函数注册菜单命令集，命令名称为 menuentry、submenu，命令的执行函
*     数为【grub-2.00/grub-core/commands/menuentry.c】的 grub_cmd_menuentry 函数。
* 6）	将输出函数指针保存到 grub_xputs_saved再将输出函数指针设定为 grub_xputs_normal()，从
*     而将输出定向到当前的终端上。
* 7）	将 normal 模块的引用计数加一，这个动作会让 normal 模块不会被卸载（因为normal 模块不
*     可被卸载）。
* 8）	注册清除命令，用于清除屏幕，处理函数为【grub-2.00/grub-core/normal/main.c】的
*     grub_mini_cmd_clear(),实际就是调用grub_cls ()。
* 9）	调用grub_set_history()函数设定命令历史深度为GRUB_DEFAULT_HISTORY_SIZE (定义为50) 个
*     命令。
* 10）	调用grub_register_variable_hook()函数设定环境变量 paper 的钩子函数，读取钩子函数
*     为空，写入钩子函数为【grub-2.00/grub-core/normal/main.c】的 grub_env_write_pager 函数。
* 11）调用grub_env_export()函数设定环境变量 paper 成为全局环境变量，意即可更改的环境变量。
* 12）调用grub_register_command()函数注册normal 命令，其执行函数为 
*    【grub-2.00/grub-core/normal/main.c】的grub_cmd_normal() 函数，用于从 rescure 模式进入
*     normal 模式。
* 13）调用grub_register_command()函数注册normal_exit 命令，其执行函数为 
*    【grub-2.00/grub-core/normal/main.c】的grub_cmd_normal_exit() 函数，用于离开 normal 模式
*    （即进入rescure 模式）。
* 14）调用grub_register_variable_hook()函数设定环境变量 color_normal 和color_highlight的
*     写入钩子函数分别为【grub-2.00/grub-core/normal/color.c】的grub_env_write_color_normal()
*     和grub_env_write_color_highlight()。
* 15）调用grub_env_export()函数设定环境变量 color_normal 和color_highlight成为全局环境变量。
* 16）设定环境变量 color_normal 的颜色值，前景颜色为白色，背景颜色为黑色；并设定环境变量
*     color_highlight 的颜色值，前景颜色为黑色，背景颜色为白色。
*
* 下表总结了GRUB_MOD_INIT(normal)中注册的一些关键命令：
*
* - 命令							功能														处理函数
* - authenticate			检查用户是否在USERLIST之中			grub_cmd_authenticate ()
* - export						导出变量												grub_cmd_export ()
* - break							跳出循环												grub_script_break ()
* - continue					继续循环												grub_script_break ()
* - shift							移动定位参数（$0, $1, $2, ...）	grub_script_shift()
* - setparams					设置定位参数										grub_script_setparams()
* - return						从一个函数返回(于bash的语义一致)grub_script_return()
* - menuentry					定义一个菜单项									grub_cmd_menuentry()
* - submenu						定义一个子菜单									grub_cmd_menuentry()
* - clear							清屏幕													grub_mini_cmd_clear()
* - normal						进入normal模式									grub_cmd_normal()
* - normal_exit				退出normal模式									grub_cmd_normal_exit()
**/
GRUB_MOD_INIT(normal)
{
  unsigned i;

  /* Previously many modules depended on gzio. Be nice to user and load it.  */
  grub_dl_load ("gzio");
  grub_errno = 0;

  grub_normal_auth_init ();
  grub_context_init ();
  grub_script_init ();
  grub_menu_init ();

  grub_xputs_saved = grub_xputs;
  grub_xputs = grub_xputs_normal;

  /* Normal mode shouldn't be unloaded.  */
  if (mod)
    grub_dl_ref (mod);

  cmd_clear =
    grub_register_command ("clear", grub_mini_cmd_clear,
			   0, N_("Clear the screen."));

  grub_set_history (GRUB_DEFAULT_HISTORY_SIZE);

  grub_register_variable_hook ("pager", 0, grub_env_write_pager);
  grub_env_export ("pager");

  /* Register a command "normal" for the rescue mode.  */
  grub_register_command ("normal", grub_cmd_normal,
			 0, N_("Enter normal mode."));
  grub_register_command ("normal_exit", grub_cmd_normal_exit,
			 0, N_("Exit from normal mode."));

  /* Reload terminal colors when these variables are written to.  */
  grub_register_variable_hook ("color_normal", NULL, grub_env_write_color_normal);
  grub_register_variable_hook ("color_highlight", NULL, grub_env_write_color_highlight);

  /* Preserve hooks after context changes.  */
  grub_env_export ("color_normal");
  grub_env_export ("color_highlight");

  /* Set default color names.  */
  grub_env_set ("color_normal", "white/black");
  grub_env_set ("color_highlight", "black/white");

  for (i = 0; i < ARRAY_SIZE (features); i++)
    {
      grub_env_set (features[i], "y");
      grub_env_export (features[i]);
    }
  grub_env_set ("grub_cpu", GRUB_TARGET_CPU);
  grub_env_export ("grub_cpu");
  grub_env_set ("grub_platform", GRUB_PLATFORM);
  grub_env_export ("grub_platform");
}

GRUB_MOD_FINI(normal)
{
  grub_context_fini ();
  grub_script_fini ();
  grub_menu_fini ();
  grub_normal_auth_fini ();

  grub_xputs = grub_xputs_saved;

  grub_set_history (0);
  grub_register_variable_hook ("pager", 0, 0);
  grub_fs_autoload_hook = 0;
  grub_unregister_command (cmd_clear);
}
