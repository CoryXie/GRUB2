/* rescue_reader.c - rescue mode reader  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009,2010  Free Software Foundation, Inc.
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
#include <grub/reader.h>
#include <grub/parser.h>
#include <grub/misc.h>
#include <grub/term.h>
#include <grub/mm.h>

#define GRUB_RESCUE_BUF_SIZE	256

static char linebuf[GRUB_RESCUE_BUF_SIZE];
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
* @brief 读入用户输入命令行。
*
* @note 注释详细内容:
*
* 本函数实现读入用户输入命令行的功能。根据控制值cont，首先打印一个行首"grub rescue> "或
* 者"> "，然后读入用户输入的命令行字符串，并返回到输出参数**line中。
**/
/* Prompt to input a command and read the line.  */
static grub_err_t
grub_rescue_read_line (char **line, int cont)
{
  int c;
  int pos = 0;
  char str[4];

  grub_printf ((cont) ? "> " : "grub rescue> ");
  grub_memset (linebuf, 0, GRUB_RESCUE_BUF_SIZE);

  while ((c = grub_getkey ()) != '\n' && c != '\r')
    {
      if (grub_isprint (c))
	{
	  if (pos < GRUB_RESCUE_BUF_SIZE - 1)
	    {
	      str[0] = c;
	      str[1] = 0;
	      linebuf[pos++] = c;
	      grub_xputs (str);
	    }
	}
      else if (c == '\b')
	{
	  if (pos > 0)
	    {
	      str[0] = c;
	      str[1] = ' ';
	      str[2] = c;
	      str[3] = 0;
	      linebuf[--pos] = 0;
	      grub_xputs (str);
	    }
	}
      grub_refresh ();
    }

  grub_xputs ("\n");
  grub_refresh ();

  *line = grub_strdup (linebuf);

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
* @brief 救援模式命令行循环。
*
* @note 注释详细内容:
*
* 本函数实现救援模式命令行循环的功能。该循环不断调用grub_rescue_read_line()读入命令行输
* 入，并调用grub_rescue_parse_line()来解析并执行该命令行输入。
**/
void __attribute__ ((noreturn))
grub_rescue_run (void)
{
  grub_printf ("Entering rescue mode...\n");

  while (1)
    {
      char *line;

      /* Print an error, if any.  */
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;

      grub_rescue_read_line (&line, 0);
      if (! line || line[0] == '\0')
	continue;

      grub_rescue_parse_line (line, grub_rescue_read_line);
      grub_free (line);
    }
}
