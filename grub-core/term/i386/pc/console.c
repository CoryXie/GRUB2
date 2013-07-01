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

#include <grub/machine/memory.h>
#include <grub/machine/console.h>
#include <grub/term.h>
#include <grub/types.h>
#include <grub/machine/int.h>

static void
int10_9 (grub_uint8_t ch, grub_uint16_t n)
{
  struct grub_bios_int_registers regs;

  regs.eax = ch | 0x0900;
  regs.ebx = grub_console_cur_color & 0xff;
  regs.ecx = n;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;  
  grub_bios_interrupt (0x10, &regs);
}

/*
 * BIOS call "INT 10H Function 03h" to get cursor position
 *	Call with	%ah = 0x03
 *			%bh = page
 *      Returns         %ch = starting scan line
 *                      %cl = ending scan line
 *                      %dh = row (0 is top)
 *                      %dl = column (0 is left)
 */


static grub_uint16_t
grub_console_getxy (struct grub_term_output *term __attribute__ ((unused)))
{
  struct grub_bios_int_registers regs;

  regs.eax = 0x0300;
  regs.ebx = 0;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;  
  grub_bios_interrupt (0x10, &regs);

  return ((regs.edx & 0xff) << 8) | ((regs.edx & 0xff00) >> 8);
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
* @brief 定位到指定的终端位置。
*
* @note 注释详细内容:
*
* 这个函数实际也是使用BIOS中断调用"INT 10H Function 02h"来实现的，将x,y封装到BIOS调用输
* 入参数的EDX寄存器中。
**/
/*
 * BIOS call "INT 10H Function 02h" to set cursor position
 *	Call with	%ah = 0x02
 *			%bh = page
 *                      %dh = row (0 is top)
 *                      %dl = column (0 is left)
 */
static void
grub_console_gotoxy (struct grub_term_output *term __attribute__ ((unused)),
		     grub_uint8_t x, grub_uint8_t y)
{
  struct grub_bios_int_registers regs;

  /* set page to 0 */
  regs.ebx = 0;
  regs.eax = 0x0200;
  regs.edx = (y << 8) | x;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;  
  grub_bios_interrupt (0x10, &regs);
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
* @brief 输出字符。
*
* @note 注释详细内容:
*
* 下面的输出字符的代码有一段解释，说明了输出的过程（这样做是因为没有很好的办法能得到屏幕
* 的高度，而TELETYPE OUTPUT BIOS调用也不能支持带属性地输出字符）：
* 
* 1）	如果要输出的是控制字符(CR, LF, BEL, BS)，直接使用INT 10, AH = 0Eh，(TELETYPE OUTPUT)；
* 2）	否则，先保存当前的位置（为原位置），如果当前的位置到了行尾，则输出空格，并再次保存新
*     的当前位置；
* 3）	然后再用int10_9()来实际输出带有属性的字符（输出字符带有指定颜色）；
* 4）	再使用grub_console_gotoxy()回到前面保存的位置。 
**/
/*
 *
 * Put the character C on the console. Because GRUB wants to write a
 * character with an attribute, this implementation is a bit tricky.
 * If C is a control character (CR, LF, BEL, BS), use INT 10, AH = 0Eh
 * (TELETYPE OUTPUT). Otherwise, save the original position, put a space,
 * save the current position, restore the original position, write the
 * character and the attribute, and restore the current position.
 *
 * The reason why this is so complicated is that there is no easy way to
 * get the height of the screen, and the TELETYPE OUTPUT BIOS call doesn't
 * support setting a background attribute.
 */
static void
grub_console_putchar_real (grub_uint8_t c)
{
  struct grub_bios_int_registers regs;
  grub_uint16_t pos;

  if (c == 7 || c == 8 || c == 0xa || c == 0xd)
    {
      regs.eax = c | 0x0e00;
      regs.ebx = 0x0001;
      regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;  
      grub_bios_interrupt (0x10, &regs);
      return;
    }

  /* get the current position */
  pos = grub_console_getxy (NULL);
  
  /* check the column with the width */
  if ((pos & 0xff00) >= (79 << 8))
    {
      grub_console_putchar_real (0x0d);
      grub_console_putchar_real (0x0a);
      /* get the current position */
      pos = grub_console_getxy (NULL);
    }

  /* write the character with the attribute */
  int10_9 (c, 1);

  grub_console_gotoxy (NULL, ((pos & 0xff00) >> 8) + 1, (pos & 0xff));
}

static void
grub_console_putchar (struct grub_term_output *term __attribute__ ((unused)),
		      const struct grub_unicode_glyph *c)
{
  grub_console_putchar_real (c->base);
}

/*
 * BIOS call "INT 10H Function 09h" to write character and attribute
 *	Call with	%ah = 0x09
 *                      %al = (character)
 *                      %bh = (page number)
 *                      %bl = (attribute)
 *                      %cx = (number of times)
 */
static void
grub_console_cls (struct grub_term_output *term)
{
  /* move the cursor to the beginning */
  grub_console_gotoxy (term, 0, 0);

  /* write spaces to the entire screen */
  int10_9 (' ', 80 * 25);

  /* move back the cursor */
  grub_console_gotoxy (term, 0, 0);
}

/*
 * void grub_console_setcursor (int on)
 * BIOS call "INT 10H Function 01h" to set cursor type
 *      Call with       %ah = 0x01
 *                      %ch = cursor starting scanline
 *                      %cl = cursor ending scanline
 */
static void 
grub_console_setcursor (struct grub_term_output *term __attribute__ ((unused)),
			int on)
{
  static grub_uint16_t console_cursor_shape = 0;
  struct grub_bios_int_registers regs;

  /* check if the standard cursor shape has already been saved */
  if (!console_cursor_shape)
    {
      regs.eax = 0x0300;
      regs.ebx = 0;
      regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;  
      grub_bios_interrupt (0x10, &regs);
      console_cursor_shape = regs.ecx;
      if ((console_cursor_shape >> 8) >= (console_cursor_shape & 0xff))
	console_cursor_shape = 0x0d0e;
    }
  /* set %cx to the designated cursor shape */
  regs.ecx = on ? console_cursor_shape : 0x2000;
  regs.eax = 0x0100;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;  
  grub_bios_interrupt (0x10, &regs);
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
* @brief 获得按键值。
*
* @note 注释详细内容:
*
* 该函数使用BIOS中断来读取键盘输入，大致过程如下：
*
* 1）	使用"INT 16H Function 01H"先检查是否有按键输入；如果没有按键输入就返回
*     GRUB_TERM_NO_KEY；
* 2）	使用"INT 16H Function 00H"来实际读入按键；并根据返回的EAX的值的状态进行相应处理。
*     如果EAX的AL部分为0（即ASCII字符部分为0），则返回AH部分或上GRUB_TERM_EXTENDED（即
*    ((regs.eax >> 8) & 0xff) | GRUB_TERM_EXTENDED）；否则，如果AL部分大于空格字符' '
*    的值，则返回AL部分（即ASCII字符部分）；再否则，如果EAX的值与bypass_table中的几个
*    值中的某个匹配，也就是按下了几个特殊的按键，如制表符，回车，换行等，则返回AL部分；
*    最后，返回(regs.eax & 0xff) + (('a' - 1) | GRUB_TERM_CTRL)，即输入的常规字符。
**/
/*
 *	if there is a character pending, return it; otherwise return -1
 * BIOS call "INT 16H Function 01H" to check whether a character is pending
 *	Call with	%ah = 0x1
 *	Return:
 *		If key waiting to be input:
 *			%ah = keyboard scan code
 *			%al = ASCII character
 *			Zero flag = clear
 *		else
 *			Zero flag = set
 * BIOS call "INT 16H Function 00H" to read character from keyboard
 *	Call with	%ah = 0x0
 *	Return:		%ah = keyboard scan code
 *			%al = ASCII character
 */

static int
grub_console_getkey (struct grub_term_input *term __attribute__ ((unused)))
{
  const grub_uint16_t bypass_table[] = {
    0x0100 | '\e', 0x0f00 | '\t', 0x0e00 | '\b', 0x1c00 | '\r', 0x1c00 | '\n'
  };
  struct grub_bios_int_registers regs;
  unsigned i;

  /*
   * Due to a bug in apple's bootcamp implementation, INT 16/AH = 0 would
   * cause the machine to hang at the second keystroke. However, we can
   * work around this problem by ensuring the presence of keystroke with
   * INT 16/AH = 1 before calling INT 16/AH = 0.
   */

  regs.eax = 0x0100;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x16, &regs);
  if (regs.flags & GRUB_CPU_INT_FLAGS_ZERO)
    return GRUB_TERM_NO_KEY;

  regs.eax = 0x0000;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x16, &regs);
  if (!(regs.eax & 0xff))
    return ((regs.eax >> 8) & 0xff) | GRUB_TERM_EXTENDED;

  if ((regs.eax & 0xff) >= ' ')
    return regs.eax & 0xff;

  for (i = 0; i < ARRAY_SIZE (bypass_table); i++)
    if (bypass_table[i] == (regs.eax & 0xffff))
      return regs.eax & 0xff;

  return (regs.eax & 0xff) + (('a' - 1) | GRUB_TERM_CTRL);
}

static const struct grub_machine_bios_data_area *bios_data_area =
  (struct grub_machine_bios_data_area *) GRUB_MEMORY_MACHINE_BIOS_DATA_AREA_ADDR;

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
* @brief 获得按键状态。
*
* @note 注释详细内容:
*
* 这个函数很简单，因为BIOS调用有个特点，就是在BIOS中断调用之后，BIOS会将一个数据存入到
* 所谓的BIOS数据区域，这段区域是在低端内存部分，可以直接读取；GRUB的做法是直接将这个数
* 据区域GRUB_MEMORY_MACHINE_BIOS_DATA_AREA_ADDR强制转换为一个
* struct grub_machine_bios_data_area结构体，从而可以直接按照访问普通的数据结构那样直接
* 访问；例如这里的bios_data_area->keyboard_flag_lower就是直接得到按键状态。
**/
static int
grub_console_getkeystatus (struct grub_term_input *term __attribute__ ((unused)))
{
  /* conveniently GRUB keystatus is modelled after BIOS one.  */
  return bios_data_area->keyboard_flag_lower & ~0x80;
}

static struct grub_term_input grub_console_term_input =
  {
    .name = "console",
    .getkey = grub_console_getkey,
    .getkeystatus = grub_console_getkeystatus
  };

static struct grub_term_output grub_console_term_output =
  {
    .name = "console",
    .putchar = grub_console_putchar,
    .getwh = grub_console_getwh,
    .getxy = grub_console_getxy,
    .gotoxy = grub_console_gotoxy,
    .cls = grub_console_cls,
    .setcolorstate = grub_console_setcolorstate,
    .setcursor = grub_console_setcursor,
    .flags = GRUB_TERM_CODE_TYPE_CP437,
    .normal_color = GRUB_TERM_DEFAULT_NORMAL_COLOR,
    .highlight_color = GRUB_TERM_DEFAULT_HIGHLIGHT_COLOR,
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
* @brief 控制台初始化。
*
* @note 注释详细内容:
*
* 这里grub_console_init()分别调用grub_term_register_output()和grub_term_register_input()
* 注册了输入和输出的控制台终端，即grub_console_term_input 和grub_console_term_output结构。
* 这两个结构都是在【grub-2.00/include/grub/term.h】中定义的。
**/
void
grub_console_init (void)
{
  grub_term_register_output ("console", &grub_console_term_output);
  grub_term_register_input ("console", &grub_console_term_input);
}

void
grub_console_fini (void)
{
  grub_term_unregister_input (&grub_console_term_input);
  grub_term_unregister_output (&grub_console_term_output);
}
