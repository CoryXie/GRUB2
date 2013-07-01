/* menu.h - Menu model function prototypes and data structures. */
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

#ifndef GRUB_MENU_HEADER
#define GRUB_MENU_HEADER 1

struct grub_menu_entry_class
{
  char *name;
  struct grub_menu_entry_class *next;
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
* @brief 菜单项结构。
*
* @note 注释详细内容: 
*
* 下面对该struct grub_menu_entry结构的关键字段进行说明：
* 
* - title，主题，即描述该菜单项的简要描述;
* - id，标识符，用于区分各个菜单项；
* - restricted，受限制的菜单项，表示需要用户身份验证才能启动的菜单项；
* - users，用于验证的用户列表；
* - classes，风格类型，用于选择一个图标或者其他风格属性；
* - sourcecode，脚本代码；
* - argc和args，脚本代码的参数个数和参数字符串数组；
* - hotkey，热键，按热键会导致对应的菜单项被运行；
* - submenu，表示该菜单项为子菜单；
* - next，指向菜单项链表的下一项。
**/
/* The menu entry.  */
struct grub_menu_entry
{
  /* The title name.  */
  const char *title;

  /* The identifier.  */
  const char *id;

  /* If set means not everybody is allowed to boot this entry.  */
  int restricted;

  /* Allowed users.  */
  const char *users;

  /* The classes associated with the menu entry:
     used to choose an icon or other style attributes.
     This is a dummy head node for the linked list, so for an entry E,
     E.classes->next is the first class if it is not NULL.  */
  struct grub_menu_entry_class *classes;

  /* The sourcecode of the menu entry, used by the editor.  */
  const char *sourcecode;

  /* Parameters to be passed to menu definition.  */
  int argc;
  char **args;

  int hotkey;

  int submenu;

  /* The next element.  */
  struct grub_menu_entry *next;
};
typedef struct grub_menu_entry *grub_menu_entry_t;

/* The menu.  */
struct grub_menu
{
  /* The size of a menu.  */
  int size;

  /* The list of menu entries.  */
  grub_menu_entry_t entry_list;
};
typedef struct grub_menu *grub_menu_t;

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
* @brief 菜单视图的回调函数指针结构。
*
* @note 注释详细内容: 
*
* 下面对该struct grub_menu_execute_callback结构的关键字段进行说明：
* 
* - notify_booting，当启动某个菜单项之前会用该函数指针，用于通知用户开始启动了；
* - notify_fallback，如果前面一次启动菜单失败，会回退到下一个菜单项再次尝试，该函数指针
*   就是在开始再次尝试时调用，告诉用于通知菜单视图即将尝试新的entry；
* - notify_failure，当重试了所有的菜单项都没有成功运行时，会调用该函数指针通知最后失败。
**/
/* Callback structure menu viewers can use to provide user feedback when
   default entries are executed, possibly including fallback entries.  */
typedef struct grub_menu_execute_callback
{
  /* Called immediately before ENTRY is booted.  */
  void (*notify_booting) (grub_menu_entry_t entry, void *userdata);

  /* Called when executing one entry has failed, and another entry, ENTRY, will
     be executed as a fallback.  The implementation of this function should
     delay for a period of at least 2 seconds before returning in order to
     allow the user time to read the information before it can be lost by
     executing ENTRY.  */
  void (*notify_fallback) (grub_menu_entry_t entry, void *userdata);

  /* Called when an entry has failed to execute and there is no remaining
     fallback entry to attempt.  */
  void (*notify_failure) (void *userdata);
}
*grub_menu_execute_callback_t;

grub_menu_entry_t grub_menu_get_entry (grub_menu_t menu, int no);
int grub_menu_get_timeout (void);
void grub_menu_set_timeout (int timeout);
void grub_menu_entry_run (grub_menu_entry_t entry);
int grub_menu_get_default_entry_index (grub_menu_t menu);

void grub_menu_init (void);
void grub_menu_fini (void);

#endif /* GRUB_MENU_HEADER */
