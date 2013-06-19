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

/* The initial context.  */
static struct grub_env_context initial_context;

/* The current context.  */
struct grub_env_context *grub_current_context = &initial_context;

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
* @brief 获得一个环境变量名字字符串的hash表示。
*
* @note 注释详细内容:
*
* 本函数实现获得一个环境变量名字字符串的hash表示的功能。返回的键值可以作为环境变量hash
* table的索引。
**/
/* Return the hash representation of the string S.  */
static unsigned int
grub_env_hashval (const char *s)
{
  unsigned int i = 0;

  /* XXX: This can be done much more efficiently.  */
  while (*s)
    i += 5 * *(s++);

  return i % HASHSZ;
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
* @brief 查找一个环境变量。
*
* @note 注释详细内容:
*
* 本函数实现查找一个环境变量的功能。首先使用grub_env_hashval()获得环境变量名称的hash键值；
* 之后使用该键值作为环境变量hash table的索引，找到grub_env_var类型的链表头；接着再循环对
* 比每个表项与name是否一样，如果一样表示找到该环境变量，并返回。
**/
static struct grub_env_var *
grub_env_find (const char *name)
{
  struct grub_env_var *var;
  int idx = grub_env_hashval (name);

  /* Look for the variable in the current context.  */
  for (var = grub_current_context->vars[idx]; var; var = var->next)
    if (grub_strcmp (var->name, name) == 0)
      return var;

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
* @date 注释添加日期：2013年6月8日
*
* @brief 插入一个环境变量。
*
* @note 注释详细内容:
*
* 本函数实现插入一个环境变量的功能。首先使用grub_env_hashval()返回hash键值；然后将这个环
* 境变量添加到键值索引的hash table的头部。
**/
static void
grub_env_insert (struct grub_env_context *context,
		 struct grub_env_var *var)
{
  int idx = grub_env_hashval (var->name);

  /* Insert the variable into the hashtable.  */
  var->prevp = &context->vars[idx];
  var->next = context->vars[idx];
  if (var->next)
    var->next->prevp = &(var->next);
  context->vars[idx] = var;
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
* @brief 删除一个环境变量。
*
* @note 注释详细内容:
*
* 本函数实现删除一个环境变量的功能。将这个环境变量从hash table的链表中脱下来。
**/
static void
grub_env_remove (struct grub_env_var *var)
{
  /* Remove the entry from the variable table.  */
  *var->prevp = var->next;
  if (var->next)
    var->next->prevp = var->prevp;
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
* @brief 设置一个环境变量的值。
*
* @note 注释详细内容:
*
* 本函数实现设置一个环境变量的值的功能。首先调用grub_env_find()找到该环境变量；如果找到
* 就将该环境变量更新成新的值并返回；如果没找到，则分配一个新的环境变量，将其插入当前的
* 环境变量hash table中。
**/
grub_err_t
grub_env_set (const char *name, const char *val)
{
  struct grub_env_var *var;

  /* If the variable does already exist, just update the variable.  */
  var = grub_env_find (name);
  if (var)
    {
      char *old = var->value;

      if (var->write_hook)
	var->value = var->write_hook (var, val);
      else
	var->value = grub_strdup (val);

      if (! var->value)
	{
	  var->value = old;
	  return grub_errno;
	}

      grub_free (old);
      return GRUB_ERR_NONE;
    }

  /* The variable does not exist, so create a new one.  */
  var = grub_zalloc (sizeof (*var));
  if (! var)
    return grub_errno;

  var->name = grub_strdup (name);
  if (! var->name)
    goto fail;

  var->value = grub_strdup (val);
  if (! var->value)
    goto fail;

  grub_env_insert (grub_current_context, var);

  return GRUB_ERR_NONE;

 fail:
  grub_free (var->name);
  grub_free (var->value);
  grub_free (var);

  return grub_errno;
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
* @brief 获得一个环境变量的值。
*
* @note 注释详细内容:
*
* 本函数实现获得一个环境变量的值的功能。首先使用grub_env_find()找到该环境变量，然后调用
* 该环境变量的read_hook函数钩子；最后返回var->value作为环境变量的值。
**/
const char *
grub_env_get (const char *name)
{
  struct grub_env_var *var;

  var = grub_env_find (name);
  if (! var)
    return 0;

  if (var->read_hook)
    return var->read_hook (var, var->value);

  return var->value;
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
* @brief 取消一个环境变量。
*
* @note 注释详细内容:
*
* 本函数实现取消一个环境变量的功能。首先调用grub_env_find()找到该环境变量；如果找到
* 就将该环境变量，并且有var->read_hook || var->write_hook，那么就将该环境变量更新成""
* 并返回；否则就删除该环境变量并释放对应资源。
**/
void
grub_env_unset (const char *name)
{
  struct grub_env_var *var;

  var = grub_env_find (name);
  if (! var)
    return;

  if (var->read_hook || var->write_hook)
    {
      grub_env_set (name, "");
      return;
    }

  grub_env_remove (var);

  grub_free (var->name);
  grub_free (var->value);
  grub_free (var);
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
* @brief 遍历所有的环境变量并调用回调函数。
*
* @note 注释详细内容:
*
* 本函数实现遍历所有的环境变量并调用回调函数的功能。首先将所有的环境变量添加到一个内部
* 的排序链表上，然后循序对这个排序链表调用回调函数，然后释放该排序链表。
**/
void
grub_env_iterate (int (*func) (struct grub_env_var *var))
{
  struct grub_env_sorted_var *sorted_list = 0;
  struct grub_env_sorted_var *sorted_var;
  int i;

  /* Add variables associated with this context into a sorted list.  */
  for (i = 0; i < HASHSZ; i++)
    {
      struct grub_env_var *var;

      for (var = grub_current_context->vars[i]; var; var = var->next)
	{
	  struct grub_env_sorted_var *p, **q;

	  sorted_var = grub_malloc (sizeof (*sorted_var));
	  if (! sorted_var)
	    goto fail;

	  sorted_var->var = var;

	  for (q = &sorted_list, p = *q; p; q = &((*q)->next), p = *q)
	    {
	      if (grub_strcmp (p->var->name, var->name) > 0)
		break;
	    }

	  sorted_var->next = *q;
	  *q = sorted_var;
	}
    }

  /* Iterate FUNC on the sorted list.  */
  for (sorted_var = sorted_list; sorted_var; sorted_var = sorted_var->next)
    if (func (sorted_var->var))
      break;

 fail:

  /* Free the sorted list.  */
  for (sorted_var = sorted_list; sorted_var; )
    {
      struct grub_env_sorted_var *tmp = sorted_var->next;

      grub_free (sorted_var);
      sorted_var = tmp;
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
* @brief 注册环境变量的读写钩子函数。
*
* @note 注释详细内容:
*
* 本函数实现注册环境变量的读写钩子函数的功能。首先使用grub_env_find()找到该环境变量；
* 如果没找到就将之设置为""并再次使用grub_env_find()找到该环境变量；最后设置read_hook
* 和write_hook。
**/
grub_err_t
grub_register_variable_hook (const char *name,
			     grub_env_read_hook_t read_hook,
			     grub_env_write_hook_t write_hook)
{
  struct grub_env_var *var = grub_env_find (name);

  if (! var)
    {
      if (grub_env_set (name, "") != GRUB_ERR_NONE)
	return grub_errno;

      var = grub_env_find (name);
      /* XXX Insert an assertion?  */
    }

  var->read_hook = read_hook;
  var->write_hook = write_hook;

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
* @date 注释添加日期：2013年6月8日
*
* @brief 导出一个环境变量。
*
* @note 注释详细内容:
*
* 本函数实现导出一个环境变量的功能。首先使用grub_env_find()找到该环境变量；如果没找到，则
* 使用grub_env_set()将其设置为“”，并重新使用grub_env_find()找到该环境变量；最后设置改环境
* 变量的global标志位1。
**/
grub_err_t
grub_env_export (const char *name)
{
  struct grub_env_var *var;

  var = grub_env_find (name);
  if (! var)
    {
      grub_err_t err;

      err = grub_env_set (name, "");
      if (err)
	return err;
      var = grub_env_find (name);
    }
  var->global = 1;

  return GRUB_ERR_NONE;
}
