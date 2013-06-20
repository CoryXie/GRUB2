/* err.c - error handling routines */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2005,2007,2008  Free Software Foundation, Inc.
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

#include <grub/err.h>
#include <grub/misc.h>
#include <stdarg.h>
#include <grub/i18n.h>

#define GRUB_ERROR_STACK_SIZE	10

grub_err_t grub_errno;
char grub_errmsg[GRUB_MAX_ERRMSG];
int grub_err_printed_errors;

static struct grub_error_saved grub_error_stack_items[GRUB_ERROR_STACK_SIZE];

static int grub_error_stack_pos;
static int grub_error_stack_assert;

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
* @brief 设置全局错误号并格式化错误消息。
*
* @note 注释详细内容:
*
* 本函数实现设置全局错误号并格式化错误消息的功能。首先将参数n设置给全局错误号grub_errno；
* 然后使用grub_vsnprintf()格式化输出错误消息。
**/
grub_err_t
grub_error (grub_err_t n, const char *fmt, ...)
{
  va_list ap;

  grub_errno = n;

  va_start (ap, fmt);
  grub_vsnprintf (grub_errmsg, sizeof (grub_errmsg), _(fmt), ap);
  va_end (ap);

  return n;
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
* @brief 格式化输出错误消息并中止执行。
*
* @note 注释详细内容:
*
* 本函数实现格式化输出错误消息并中止执行的功能。首先使用grub_vprintf()格式化输出错误消息；
* 然后调用grub_abort()中止执行。
**/
void
grub_fatal (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  grub_vprintf (_(fmt), ap);
  va_end (ap);

  grub_abort ();
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
* @brief 将当前错误号和错误消息压入错误消息堆栈。
*
* @note 注释详细内容:
*
* 本函数实现将当前错误号和错误消息压入错误消息堆栈的功能。错误消息堆栈是使用数组实现的；
* 如果当前堆栈位置在grub_error_stack_pos小于数组大小，那么就将grub_errno和当前的错误消息
* 压入grub_error_stack_pos所在的数组grub_error_stack_items[]的对应元素，否则设置全局变量
* grub_error_stack_assert；最后清除grub_errno让其他模块可以继续执行。
**/
void
grub_error_push (void)
{
  /* Only add items to stack, if there is enough room.  */
  if (grub_error_stack_pos < GRUB_ERROR_STACK_SIZE)
    {
      /* Copy active error message to stack.  */
      grub_error_stack_items[grub_error_stack_pos].grub_errno = grub_errno;
      grub_memcpy (grub_error_stack_items[grub_error_stack_pos].errmsg,
                   grub_errmsg,
                   sizeof (grub_errmsg));

      /* Advance to next error stack position.  */
      grub_error_stack_pos++;
    }
  else
    {
      /* There is no room for new error message. Discard new error message
         and mark error stack assertion flag.  */
      grub_error_stack_assert = 1;
    }

  /* Allow further operation of other components by resetting
     active errno to GRUB_ERR_NONE.  */
  grub_errno = GRUB_ERR_NONE;
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
* @brief 从错误消息堆栈弹出最近的错误信息。
*
* @note 注释详细内容:
*
* 本函数实现从错误消息堆栈弹出最近的错误信息的功能。错误消息堆栈是使用数组实现的；
* 如果当前堆栈位置在grub_error_stack_pos大于0则从当前位置弹出错误消息，否则设置错
* 误号grub_errno为GRUB_ERR_NONE。
**/
int
grub_error_pop (void)
{
  if (grub_error_stack_pos > 0)
    {
      /* Pop error message from error stack to current active error.  */
      grub_error_stack_pos--;

      grub_errno = grub_error_stack_items[grub_error_stack_pos].grub_errno;
      grub_memcpy (grub_errmsg,
                   grub_error_stack_items[grub_error_stack_pos].errmsg,
                   sizeof (grub_errmsg));

      return 1;
    }
  else
    {
      /* There is no more items on error stack, reset to no error state.  */
      grub_errno = GRUB_ERR_NONE;

      return 0;
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
* @brief 反向打印所有错误堆栈中的错误信息。
*
* @note 注释详细内容:
*
* 本函数实现反向打印所有错误堆栈中的错误信息的功能。对于错误堆栈中的每条消息，如
* 果grub_errno != GRUB_ERR_NONE则打印对应的错误消息，并继续使用grub_error_pop()弹出
* 下一个错误消息，直到grub_error_pop()返回0表示没有已经存储的错误消息。如果曾经出
* 现错误消息满（grub_error_stack_assert被设置的情况），则打印一条错误堆栈溢出的消息
* 并重置grub_error_stack_assert。
**/
void
grub_print_error (void)
{
  /* Print error messages in reverse order. First print active error message
     and then empty error stack.  */
  do
    {
      if (grub_errno != GRUB_ERR_NONE)
	{
	  grub_err_printf (_("error: %s.\n"), grub_errmsg);
	  grub_err_printed_errors++;
	}
    }
  while (grub_error_pop ());

  /* If there was an assert while using error stack, report about it.  */
  if (grub_error_stack_assert)
    {
      grub_err_printf ("assert: error stack overflow detected!\n");
      grub_error_stack_assert = 0;
    }
}
