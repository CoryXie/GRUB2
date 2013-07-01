/* kern/i386/tsc.c - x86 TSC time source implementation
 * Requires Pentium or better x86 CPU that supports the RDTSC instruction.
 * This module uses the RTC (via grub_get_rtc()) to calibrate the TSC to
 * real time.
 *
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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
#include <grub/time.h>
#include <grub/misc.h>
#include <grub/i386/tsc.h>
#include <grub/i386/pit.h>

/* This defines the value TSC had at the epoch (that is, when we calibrated it). */
static grub_uint64_t tsc_boot_time;

/* Calibrated TSC rate.  (In TSC ticks per millisecond.) */
static grub_uint64_t tsc_ticks_per_ms;

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
* @brief 特定于i386-pc架构的TSC时间取得函数。
*
* @note 注释详细内容:
*
* 本函数实现特定于i386-pc架构的TSC时间取得函数的功能。这是I386-PC 架构独有的时间取得函数，
* 取得CPU 时间值后，再转换成毫秒为单位的时间值。其中函数grub_get_tsc() 取得CPU时间戳计值，
* tsc_ticks_per_ms 是时间戳计值转换成毫时间值的除数值。
**/
grub_uint64_t
grub_tsc_get_time_ms (void)
{
  return tsc_boot_time + grub_divmod64 (grub_get_tsc (), tsc_ticks_per_ms, 0);
}


/* How many RTC ticks to use for calibration loop. (>= 1) */
#define CALIBRATION_TICKS 2
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
* @brief 基于RTC的时间戳校准函数。
*
* @note 注释详细内容:
*
* 本函数实现基于RTC的时间戳校准函数的功能。函数首先取得CPU时间戳值，这是时间起始值；然后
* 使用PIT等待跳过0xffff的时间值;接着再次取得CPU时间戳值，这是时间终了值。最后时间戳的差异
* 除以时间值55毫秒，算得每毫秒的时间戳计时次数，存入tsc_ticks_per_ms中。
**/
/* Calibrate the TSC based on the RTC.  */
static void
calibrate_tsc (void)
{
  /* First calibrate the TSC rate (relative, not absolute time). */
  grub_uint64_t start_tsc;
  grub_uint64_t end_tsc;

  start_tsc = grub_get_tsc ();
  grub_pit_wait (0xffff);
  end_tsc = grub_get_tsc ();

  tsc_ticks_per_ms = grub_divmod64 (end_tsc - start_tsc, 55, 0);
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
* @brief 时间戳初始化函数。
*
* @note 注释详细内容:
*
* 本函数实现时间戳初始化函数的功能。函数在CPU支持时间戳时，取得目前时间戳计值，这是时间
* 起始值，然后调用时间校准函数calibrate_tsc()，取得毫秒时间戳计时次数值；然后安装时间戳
* 计时间取得函式成为时间管理器的时间取得函数。如果CPU没有支持时间戳，使用RTC的时间取得
* 函数grub_rtc_get_time_ms做为时间管理器的时间取得函数。该函数首先判断机器是否支持TSC。
* 时间戳计数器（TSC）是一个64位寄存器，存在于所有奔腾以来的x86处理器。它从复位开始就对
* 机器的周期数进行计数。指令RDTSC返回TSC的值到 EDX:EAX中。在x86-64模式时，RDTSC也清除
* RAX的高32位。使用时间戳计数器（TSC），可以以出色的高分辨率，低开销的方式获得CPU时序
* 信息。因此，如果机器支持TSC，就会尽量使用TSC作为计时方式。此时，第一件事就是调用
* grub_get_tsc ()来获得GRUB2启动的时间tsc_boot_time。接着，调用calibrate_tsc ()来校正
* TSC，获得tsc_ticks_per_ms，表示没毫秒TSC会跳变多少次（这个校正是通过PIT来进行的）。
**/
void
grub_tsc_init (void)
{
  if (grub_cpu_is_tsc_supported ())
    {
      tsc_boot_time = grub_get_tsc ();
      calibrate_tsc ();
      grub_install_get_time_ms (grub_tsc_get_time_ms);
    }
  else
    {
#if defined (GRUB_MACHINE_PCBIOS) || defined (GRUB_MACHINE_IEEE1275)
      grub_install_get_time_ms (grub_rtc_get_time_ms);
#else
      grub_fatal ("no TSC found");
#endif
    }
}
