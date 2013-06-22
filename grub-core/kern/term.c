/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/term.h>
#include <grub/err.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/time.h>

struct grub_term_output *grub_term_outputs_disabled;
struct grub_term_input *grub_term_inputs_disabled;
struct grub_term_output *grub_term_outputs;
struct grub_term_input *grub_term_inputs;

void (*grub_term_poll_usb) (void) = NULL;
void (*grub_net_poll_cards_idle) (void) = NULL;

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
* @brief 输出一个Unicode字符。
*
* @note 注释详细内容:
*
* 本函数实现输出一个Unicode字符的功能。将实际的要输出的字符保存在grub_unicode_glyph结构中，
* 然后调用终端的输出字符换上完成实际的输出。遇到'\t'或者'\n'会做相应扩展，递归调用本函数
* 来扩展。
**/
/* Put a Unicode character.  */
static void
grub_putcode_dumb (grub_uint32_t code,
		   struct grub_term_output *term)
{
  struct grub_unicode_glyph c =
    {
      .base = code,
      .variant = 0,
      .attributes = 0,
      .ncomb = 0,
      .combining = 0,
      .estimated_width = 1
    };

  if (code == '\t' && term->getxy)
    {
      int n;

      n = GRUB_TERM_TAB_WIDTH - ((term->getxy (term) >> 8)
				 % GRUB_TERM_TAB_WIDTH);
      while (n--)
	grub_putcode_dumb (' ', term);

      return;
    }

  (term->putchar) (term, &c);
  if (code == '\n')
    grub_putcode_dumb ('\r', term);
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
* @brief 输出字符串。
*
* @note 注释详细内容:
*
* 本函数实现输出字符串的功能。
**/
static void
grub_xputs_dumb (const char *str)
{
  for (; *str; str++)
    {
      grub_term_output_t term;
      grub_uint32_t code = *str;
      if (code > 0x7f)
	code = '?';

      FOR_ACTIVE_TERM_OUTPUTS(term)
	grub_putcode_dumb (code, term);
    }
}

void (*grub_xputs) (const char *str) = grub_xputs_dumb;
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
* @brief 非阻塞地获得一个字符输入。
*
* @note 注释详细内容:
*
* 本函数非阻塞地获得一个字符输入的功能。如果有USB串口输入，则调用grub_term_poll_usb()；
* 如果有网卡输入，则调用grub_net_poll_cards_idle()；之后使用FOR_ACTIVE_TERM_INPUTS()对
* 每个终端调用getkey()来读取输入字符。如果读到有字符，则返回读入的字符；否则就返回没有
* 读入的标志GRUB_TERM_NO_KEY。
**/
int
grub_getkey_noblock (void)
{
  grub_term_input_t term;

  if (grub_term_poll_usb)
    grub_term_poll_usb ();

  if (grub_net_poll_cards_idle)
    grub_net_poll_cards_idle ();

  FOR_ACTIVE_TERM_INPUTS(term)
  {
    int key = term->getkey (term);
    if (key != GRUB_TERM_NO_KEY)
      return key;
  }

  return GRUB_TERM_NO_KEY;
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
* @brief 阻塞地获得一个字符输入。
*
* @note 注释详细内容:
*
* 本函数阻塞地获得一个字符输入的功能。循环地调用grub_getkey_noblock()直到该函数返回
* 一个有效的字符输入（不是GRUB_TERM_NO_KEY）。
**/
int
grub_getkey (void)
{
  int ret;

  grub_refresh ();

  while (1)
    {
      ret = grub_getkey_noblock ();
      if (ret != GRUB_TERM_NO_KEY)
	return ret;
      grub_cpu_idle ();
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
* @date 注释添加日期：2013年6月21日
*
* @brief 刷新终端。
*
* @note 注释详细内容:
*
* 本函数刷新终端的功能。调用FOR_ACTIVE_TERM_OUTPUTS()对每个终端调用grub_term_refresh()。
**/
void
grub_refresh (void)
{
  struct grub_term_output *term;

  FOR_ACTIVE_TERM_OUTPUTS(term)
    grub_term_refresh (term);
}
