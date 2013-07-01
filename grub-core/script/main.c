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

#include <grub/dl.h>
#include <grub/i18n.h>
#include <grub/parser.h>
#include <grub/script_sh.h>

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
* @brief 解析并处理一行用户输入。
*
* @note 注释详细内容:
*
* 函数grub_normal_parse_line()负责解析并处理一行用户输入。注意到传递给函数grub_normal_parse_line()
* 的getline为grub_normal_read_line_real()，因此可以预见，在执行grub_normal_parse_line()的
* 过程中如果还需要用户输入，就会调用这个函数，从而用上了该函数中的cont参数（显示“>”让用户
* 觉得是在继续输入需要的字符串）。该函数首先调用grub_script_parse()来解析输入命令行，返回一个
* struct grub_script代表的脚步（命令集合）；如果的确解析出了要执行的命令集合，就会调用
* grub_script_execute()执行命令脚本；之后再用grub_script_unref()来去掉该脚本的定义。
**/
grub_err_t
grub_normal_parse_line (char *line, grub_reader_getline_t getline)
{
  struct grub_script *parsed_script;

  /* Parse the script.  */
  parsed_script = grub_script_parse (line, getline);

  if (parsed_script)
    {
      /* Execute the command(s).  */
      grub_script_execute (parsed_script);

      /* The parsed script was executed, throw it away.  */
      grub_script_unref (parsed_script);
    }

  return grub_errno;
}

static grub_command_t cmd_break;
static grub_command_t cmd_continue;
static grub_command_t cmd_shift;
static grub_command_t cmd_setparams;
static grub_command_t cmd_return;

void
grub_script_init (void)
{
  cmd_break = grub_register_command ("break", grub_script_break,
				     N_("[NUM]"), N_("Exit from loops"));
  cmd_continue = grub_register_command ("continue", grub_script_break,
					N_("[NUM]"), N_("Continue loops"));
  cmd_shift = grub_register_command ("shift", grub_script_shift,
				     N_("[NUM]"),
				     /* TRANSLATORS: Positional arguments are
					arguments $0, $1, $2, ...  */
				     N_("Shift positional parameters."));
  cmd_setparams = grub_register_command ("setparams", grub_script_setparams,
					 N_("[VALUE]..."),
					 N_("Set positional parameters."));
  cmd_return = grub_register_command ("return", grub_script_return,
				      N_("[NUM]"),
				      /* TRANSLATORS: It's a command description
					 and "Return" is a verb, not a noun. The
					 command in question is "return" and
					 has exactly the same semanics as bash
					 equivalent.  */
				      N_("Return from a function."));
}

void
grub_script_fini (void)
{
  if (cmd_break)
    grub_unregister_command (cmd_break);
  cmd_break = 0;

  if (cmd_continue)
    grub_unregister_command (cmd_continue);
  cmd_continue = 0;

  if (cmd_shift)
    grub_unregister_command (cmd_shift);
  cmd_shift = 0;

  if (cmd_setparams)
    grub_unregister_command (cmd_setparams);
  cmd_setparams = 0;

  if (cmd_return)
    grub_unregister_command (cmd_return);
  cmd_return = 0;
}
