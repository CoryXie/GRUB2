/* list.c - grub list function */
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

#include <grub/list.h>
#include <grub/misc.h>
#include <grub/mm.h>

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
* @brief 使用名字在命名链表上查找一个链表项。
*
* @note 注释详细内容:
*
* 本函数实现使用名字在命名链表上查找一个链表项的功能。使用FOR_LIST_ELEMENTS()扫描链表的
* 每一项并使用grub_strcmp()对名字进行比较，从而得出找到的项。
**/
void *
grub_named_list_find (grub_named_list_t head, const char *name)
{
  grub_named_list_t item;

  FOR_LIST_ELEMENTS (item, head)
    if (grub_strcmp (item->name, name) == 0)
      return item;

  return NULL;
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
* @brief 将新项目插入到命名链表的头部。
*
* @note 注释详细内容:
*
* 本函数实现将新项目插入到命名链表的头部的功能。
**/
void
grub_list_push (grub_list_t *head, grub_list_t item)
{
  item->prev = head;
  if (*head)
    (*head)->prev = &item->next;
  item->next = *head;
  *head = item;
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
* @brief 将项目从命名链表中删除。
*
* @note 注释详细内容:
*
* 本函数实现将项目从命名链表中删除的功能。
**/
void
grub_list_remove (grub_list_t item)
{
  if (item->prev)
    *item->prev = item->next;
  if (item->next)
    item->next->prev = item->prev;
  item->next = 0;
  item->prev = 0;
}
