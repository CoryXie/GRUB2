/* env.c - Environment variables */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/env.h>
#include <grub/env_private.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/command.h>
#include <grub/normal.h>
#include <grub/i18n.h>

struct menu_pointer
{
  grub_menu_t menu;
  struct menu_pointer *prev;
};

static struct menu_pointer initial_menu;
static struct menu_pointer *current_menu = &initial_menu;

void
grub_env_unset_menu (void)
{
  current_menu->menu = NULL;
}

grub_menu_t
grub_env_get_menu (void)
{
  return current_menu->menu;
}

void
grub_env_set_menu (grub_menu_t nmenu)
{
  current_menu->menu = nmenu;
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
* @brief 新建菜单环境上下文。
*
* @note 注释详细内容:
*
* 该函数分配struct grub_env_context *context和struct menu_pointer *menu各一个，并将分配出
* 来的context和menu分别加到grub_current_context和current_menu的链表的链表首部。然后，对于
* 链表grub_current_context的下一个元素，也就是原来的context，将该context上的所有global标
* 记为1的环境变量复制到当前环境变量上下文中，或者当参数export_all非0，复制所有的前一个
* context的环境变量。
**/
static grub_err_t
grub_env_new_context (int export_all)
{
  struct grub_env_context *context;
  int i;
  struct menu_pointer *menu;

  context = grub_zalloc (sizeof (*context));
  if (! context)
    return grub_errno;
  menu = grub_zalloc (sizeof (*menu));
  if (! menu)
    return grub_errno;

  context->prev = grub_current_context;
  grub_current_context = context;

  menu->prev = current_menu;
  current_menu = menu;

  /* Copy exported variables.  */
  for (i = 0; i < HASHSZ; i++)
    {
      struct grub_env_var *var;

      for (var = context->prev->vars[i]; var; var = var->next)
	if (var->global || export_all)
	  {
	    if (grub_env_set (var->name, var->value) != GRUB_ERR_NONE)
	      {
		grub_env_context_close ();
		return grub_errno;
	      }
	    grub_env_export (var->name);
	    grub_register_variable_hook (var->name, var->read_hook, var->write_hook);
	  }
    }

  return GRUB_ERR_NONE;
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
* @brief 打开菜单环境上下文。
*
* @note 注释详细内容:
*
* 该函数调用grub_env_new_context ()，并传递参数export_all为0。该函数与grub_env_context_close()
* 成对使用。
*
* 这里对环境变量的使用做进一步说明：
* 
* 环境变量和GRUB的菜单支持相关联的。
* 
* 实际上，grub_env_context_open ()和grub_env_context_close()是在打开和关闭菜单(menu)的过程中成对
* 使用的。
* 
* 当打开一个子菜单的时候，调用grub_env_context_open ()进而调用函数grub_env_new_context ()；而
* grub_env_new_context ()函数分配新的context和menu各一个，并将分配出来的context和menu分别加到
* grub_current_context和current_menu的链表的链表首部然后将新的context和menu设置为
* grub_current_context和current_menu。也就是说，实际上创建了一个所谓的“当前上下文”和“当前菜单”。 
* 然后，对于链表grub_current_context的下一个元素，也就是原来的context，将该context上的所有global
* 标记为1的环境变量复制到当前环境变量上下文中，或者当参数export_all非0，复制所有的前一个context
* 的环境变量。因此，从initial_context开始，只要有global的环境变量，会在一级一级不断打开新的子菜
* 单的过程中递归复制到最下一级的子菜单中。
* 
* 当从一个子菜单返回时，调用grub_env_context_close()，就会释放掉“当前环境变量上下文
* ”grub_current_context的所有变量；并且将原来的next设置为grub_current_context，也就是恢复上一级
* 菜单对应的“当前环境变量上下文”。整个过程实现了环境变量的递归传递和释放。而环境变量的global属
* 性的意义也在于此。
**/
grub_err_t
grub_env_context_open (void)
{
  return grub_env_new_context (0);
}

int grub_extractor_level = 0;

grub_err_t
grub_env_extractor_open (int source)
{
  grub_extractor_level++;
  return grub_env_new_context (source);
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
* @brief 关闭菜单环境上下文。
*
* @note 注释详细内容:
*
* 该函数释放当前环境变量上下文grub_current_context中所有的环境变量。并将grub_current_context
* 替换为原来的(next)环境变量。该函数与grub_env_context_open ()成对使用。
**/
grub_err_t
grub_env_context_close (void)
{
  struct grub_env_context *context;
  int i;
  struct menu_pointer *menu;

  if (! grub_current_context->prev)
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
		       "cannot close the initial context");

  /* Free the variables associated with this context.  */
  for (i = 0; i < HASHSZ; i++)
    {
      struct grub_env_var *p, *q;

      for (p = grub_current_context->vars[i]; p; p = q)
	{
	  q = p->next;
          grub_free (p->name);
	  grub_free (p->value);
	  grub_free (p);
	}
    }

  /* Restore the previous context.  */
  context = grub_current_context->prev;
  grub_free (grub_current_context);
  grub_current_context = context;

  menu = current_menu->prev;
  grub_free (current_menu);
  current_menu = menu;

  return GRUB_ERR_NONE;
}

grub_err_t
grub_env_extractor_close (int source)
{
  grub_menu_t menu = NULL;
  grub_menu_entry_t *last;
  grub_err_t err;

  if (source)
    {
      menu = grub_env_get_menu ();
      grub_env_unset_menu ();
    }
  err = grub_env_context_close ();

  if (source && menu)
    {
      grub_menu_t menu2;
      menu2 = grub_env_get_menu ();
      
      last = &menu2->entry_list;
      while (*last)
	last = &(*last)->next;
      
      *last = menu->entry_list;
      menu2->size += menu->size;
    }

  grub_extractor_level--;
  return err;
}

static grub_command_t export_cmd;

static grub_err_t
grub_cmd_export (struct grub_command *cmd __attribute__ ((unused)),
		 int argc, char **args)
{
  int i;

  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
		       N_("one argument expected"));

  for (i = 0; i < argc; i++)
    grub_env_export (args[i]);

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
* @date 注释添加日期：2013年6月28日
*
* @brief 初始化菜单环境上下文。
*
* @note 注释详细内容:
*
* 通过grub_register_command()注册了一个新的命令 “export”,对应的命令处理函数是grub_cmd_export()，
* 该处理函数实际上就是调用grub_env_export()对参数指定的环境变量进行导出。该命令通过
* grub_context_fini()被注销。
**/
void
grub_context_init (void)
{
  export_cmd = grub_register_command ("export", grub_cmd_export,
				      N_("ENVVAR [ENVVAR] ..."),
				      N_("Export variables."));
}

void
grub_context_fini (void)
{
  grub_unregister_command (export_cmd);
}
