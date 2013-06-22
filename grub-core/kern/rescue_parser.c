/* rescue_parser.c - rescue mode parser  */
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

#include <grub/types.h>
#include <grub/mm.h>
#include <grub/env.h>
#include <grub/parser.h>
#include <grub/misc.h>
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
* @date 注释添加日期：2013年6月21日
*
* @brief 解析并执行单行命令输入。
*
* @note 注释详细内容:
*
* 本函数实现解析并执行单行命令输入的功能。调用grub_parser_split_cmdline()对参数line指定
* 的命令行进行分析，得出该命令行中有的命令参数个数和命令参数数组；如果命令参数只有一个，
* 那么如果是"variable=value"格式的设置变量的话，就使用grub_env_set()来设置对应的变量；
* 否则使用第一个参数作为命令名字，调用grub_command_find()去查找是否有对应的命令，如果找
* 到则调用cmd->func()来执行该命令。
**/
grub_err_t
grub_rescue_parse_line (char *line, grub_reader_getline_t getline)
{
  char *name;
  int n;
  grub_command_t cmd;
  char **args;

  if (grub_parser_split_cmdline (line, getline, &n, &args) || n < 0)
    return grub_errno;

  if (n == 0)
    return GRUB_ERR_NONE;

  /* In case of an assignment set the environment accordingly
     instead of calling a function.  */
  if (n == 1 && grub_strchr (line, '='))
    {
      char *val = grub_strchr (args[0], '=');
      val[0] = 0;
      grub_env_set (args[0], val + 1);
      val[0] = '=';
      goto quit;
    }

  /* Get the command name.  */
  name = args[0];

  /* If nothing is specified, restart.  */
  if (*name == '\0')
    goto quit;

  cmd = grub_command_find (name);
  if (cmd)
    {
      (cmd->func) (cmd, n - 1, &args[1]);
    }
  else
    {
      grub_printf_ (N_("Unknown command `%s'.\n"), name);
      if (grub_command_find ("help"))
	grub_printf ("Try `help' for usage\n");
    }

 quit:
  grub_free (args[0]);
  grub_free (args);

  return grub_errno;
}
