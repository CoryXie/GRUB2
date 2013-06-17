/* command.c - support basic command */
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

#include <grub/mm.h>
#include <grub/command.h>

grub_command_t grub_command_list;
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
* @brief 按名字顺序和优先级顺序将命令加入命令列表。
*
* @note 注释详细内容:
*
* 本函数实现按名字顺序和优先级顺序将命令加入命令列表的功能。
*
* 该函数扫描grub_command_list，按照命令名字顺序插入该链表；如果发现相同名字的
* 命令，且如果函数指定的这个新命令的优先级比已有的命令优先级更高，则将原有命令
* 设为"不活动"(清除掉GRUB_COMMAND_FLAG_ACTIVE位)并退出循环，然后将新命令插入在
* 这个同名的名字前，然后将新命令设置为"活动"(设置GRUB_COMMAND_FLAG_ACTIVE)。
**/

grub_command_t
grub_register_command_prio (const char *name,
			    grub_command_func_t func,
			    const char *summary,
			    const char *description,
			    int prio)
{
  grub_command_t cmd;
  int inactive = 0;

  grub_command_t *p, q;

  cmd = (grub_command_t) grub_zalloc (sizeof (*cmd));
  if (! cmd)
    return 0;

  cmd->name = name;
  cmd->func = func;
  cmd->summary = (summary) ? summary : "";
  cmd->description = description;

  cmd->flags = 0;
  cmd->prio = prio;

  for (p = &grub_command_list, q = *p; q; p = &(q->next), q = q->next)
    {
      int r;

      r = grub_strcmp (cmd->name, q->name);
      if (r < 0)
	break;
      if (r > 0)
	continue;

      if (cmd->prio >= (q->prio & GRUB_COMMAND_PRIO_MASK))
	{
	  q->prio &= ~GRUB_COMMAND_FLAG_ACTIVE;
	  break;
	}

      inactive = 1;
    }

  *p = cmd;
  cmd->next = q;
  if (q)
    q->prev = &cmd->next;
  cmd->prev = p;

  if (! inactive)
    cmd->prio |= GRUB_COMMAND_FLAG_ACTIVE;

  return cmd;
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
* @brief 取消注册一个命令。
*
* @note 注释详细内容:
*
* 本函数实现取消注册一个命令的功能。
*
* 如果该命令当前是活动的，且还有下一个命令，那么将下一个命令设为活动状态；
* 接着讲该命令从命令列表删除，并释放该命令的内存。
**/

void
grub_unregister_command (grub_command_t cmd)
{
  if ((cmd->prio & GRUB_COMMAND_FLAG_ACTIVE) && (cmd->next))
    cmd->next->prio |= GRUB_COMMAND_FLAG_ACTIVE;
  grub_list_remove (GRUB_AS_LIST (cmd));
  grub_free (cmd);
}
