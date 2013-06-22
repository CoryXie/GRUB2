/* time.c - kernel time functions */
/*
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

#include <grub/time.h>

typedef grub_uint64_t (*get_time_ms_func_t) (void);

/* Function pointer to the implementation in use.  */
static get_time_ms_func_t get_time_ms_func;

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
* @brief 时间取得函数。
*
* @note 注释详细内容:
*
* 本函数实现时间取得函数的功能。因为不同架构的时间计算函数不一样，所以再调用一层架构有关
* 的时间值取得函数。get_time_ms_func 是一个函数指针，指向不同架构的时间值取得函数。
**/
grub_uint64_t
grub_get_time_ms (void)
{
  return get_time_ms_func ();
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
* @brief 时间取得函数的安装函数。
*
* @note 注释详细内容:
*
* 本函数实现安装时间取得函数指针的功能。设定时间值取得函数指针，之后才可以透过函数指针执
* 行时间值取得函式，取得系统时间值。例如，grub-2.00/grub-core/kern/i386/tsc.c中就调用该
* 函数grub_install_get_time_ms (grub_tsc_get_time_ms)来安装一个使用TSC的实际时间取得函数。
**/
void
grub_install_get_time_ms (get_time_ms_func_t func)
{
  get_time_ms_func = func;
}
