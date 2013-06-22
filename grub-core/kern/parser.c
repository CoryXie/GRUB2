/* parser.c - the part of the parser that can return partial tokens */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007,2009  Free Software Foundation, Inc.
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

#include <grub/parser.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/mm.h>


/* All the possible state transitions on the command line.  If a
   transition can not be found, it is assumed that there is no
   transition and keep_value is assumed to be 1.  */
static struct grub_parser_state_transition state_transitions[] = {
  {GRUB_PARSER_STATE_TEXT, GRUB_PARSER_STATE_QUOTE, '\'', 0},
  {GRUB_PARSER_STATE_TEXT, GRUB_PARSER_STATE_DQUOTE, '\"', 0},
  {GRUB_PARSER_STATE_TEXT, GRUB_PARSER_STATE_VAR, '$', 0},
  {GRUB_PARSER_STATE_TEXT, GRUB_PARSER_STATE_ESC, '\\', 0},

  {GRUB_PARSER_STATE_ESC, GRUB_PARSER_STATE_TEXT, 0, 1},

  {GRUB_PARSER_STATE_QUOTE, GRUB_PARSER_STATE_TEXT, '\'', 0},

  {GRUB_PARSER_STATE_DQUOTE, GRUB_PARSER_STATE_TEXT, '\"', 0},
  {GRUB_PARSER_STATE_DQUOTE, GRUB_PARSER_STATE_QVAR, '$', 0},

  {GRUB_PARSER_STATE_VAR, GRUB_PARSER_STATE_VARNAME2, '{', 0},
  {GRUB_PARSER_STATE_VAR, GRUB_PARSER_STATE_VARNAME, 0, 1},
  {GRUB_PARSER_STATE_VARNAME, GRUB_PARSER_STATE_TEXT, ' ', 1},
  {GRUB_PARSER_STATE_VARNAME, GRUB_PARSER_STATE_TEXT, '\t', 1},
  {GRUB_PARSER_STATE_VARNAME2, GRUB_PARSER_STATE_TEXT, '}', 0},

  {GRUB_PARSER_STATE_QVAR, GRUB_PARSER_STATE_QVARNAME2, '{', 0},
  {GRUB_PARSER_STATE_QVAR, GRUB_PARSER_STATE_QVARNAME, 0, 1},
  {GRUB_PARSER_STATE_QVARNAME, GRUB_PARSER_STATE_TEXT, '\"', 0},
  {GRUB_PARSER_STATE_QVARNAME, GRUB_PARSER_STATE_DQUOTE, ' ', 1},
  {GRUB_PARSER_STATE_QVARNAME, GRUB_PARSER_STATE_DQUOTE, '\t', 1},
  {GRUB_PARSER_STATE_QVARNAME2, GRUB_PARSER_STATE_DQUOTE, '}', 0},

  {0, 0, 0, 0}
};

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
* @brief 判定命令行当前状态的下一个状态。
*
* @note 注释详细内容:
*
* 本函数实现判定命令行当前状态的下一个状态的功能。函数扫描所有的可能状态转换，找到匹配的
* 原状态（与state参数一致）和当前输入比对字符c，这两个值就决定了下一个状态，因此如果找到
* 这样的匹配，就直接退出，返回这个转换对应的新状态。如果没找到完全匹配（输入c与状态转换
* 要求的input一致），那么如果这个input要求的是空格而字符c既不是alpha，也不是数字，也不是
* 下划线，则也算是匹配，也可以返回对应的转换。再者，如果该转换要求的input为空，那么也可以
* 算一个不是很完美的匹配，也返回对应的转换。如果前面扫描找到的状态转换的原状态为空，那么
* 就只能用默认的转换（新状态与参数state一样，而且result保持输入字符的原值）。最后返回的是
* 找到的转换的新状态。
**/
/* Determines the state following STATE, determined by C.  */
grub_parser_state_t
grub_parser_cmdline_state (grub_parser_state_t state, char c, char *result)
{
  struct grub_parser_state_transition *transition;
  struct grub_parser_state_transition default_transition;

  default_transition.to_state = state;
  default_transition.keep_value = 1;

  /* Look for a good translation.  */
  for (transition = state_transitions; transition->from_state; transition++)
    {
      if (transition->from_state != state)
	continue;
      /* An exact match was found, use it.  */
      if (transition->input == c)
	break;

      if (grub_isspace (transition->input) && !grub_isalpha (c)
	  && !grub_isdigit (c) && c != '_')
	break;

      /* A less perfect match was found, use this one if no exact
         match can be found.  */
      if (transition->input == 0)
	break;
    }

  if (!transition->from_state)
    transition = &default_transition;

  if (transition->keep_value)
    *result = c;
  else
    *result = 0;
  return transition->to_state;
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
* @brief 从命令行输入文本字符串中分离出命令的参数个数和参数数组。
*
* @note 注释详细内容:
*
* 本函数实现从命令行输入文本字符串中分离出命令的参数个数和参数数组的功能。参数cmdline为
* 命令行文本字符串输入；参数getline为用来读入一行文本的回调函数；参数*argc为用来保存分离
* 出来的参数个数的输出参数；参数***argv为用来保存分离出来的参数数组的输出参数。
**/
grub_err_t
grub_parser_split_cmdline (const char *cmdline, grub_reader_getline_t getline,
			   int *argc, char ***argv)
{
  grub_parser_state_t state = GRUB_PARSER_STATE_TEXT;
  /* XXX: Fixed size buffer, perhaps this buffer should be dynamically
     allocated.  */
  char buffer[1024];
  char *bp = buffer;
  char *rd = (char *) cmdline;
  char varname[200];
  char *vp = varname;
  char *args;
  int i;

  auto int check_varstate (grub_parser_state_t s);

  int check_varstate (grub_parser_state_t s)
  {
    return (s == GRUB_PARSER_STATE_VARNAME
	    || s == GRUB_PARSER_STATE_VARNAME2
	    || s == GRUB_PARSER_STATE_QVARNAME
	    || s == GRUB_PARSER_STATE_QVARNAME2);
  }

  auto void add_var (grub_parser_state_t newstate);

  void add_var (grub_parser_state_t newstate)
  {
    const char *val;

    /* Check if a variable was being read in and the end of the name
       was reached.  */
    if (!(check_varstate (state) && !check_varstate (newstate)))
      return;

    *(vp++) = '\0';
    val = grub_env_get (varname);
    vp = varname;
    if (!val)
      return;

    /* Insert the contents of the variable in the buffer.  */
    for (; *val; val++)
      *(bp++) = *val;
  }

  *argc = 0;
  do
    {
      if (!rd || !*rd)
	{
	  if (getline)
	    getline (&rd, 1);
	  else
	    break;
	}

      if (!rd)
	break;

      for (; *rd; rd++)
	{
	  grub_parser_state_t newstate;
	  char use;

	  newstate = grub_parser_cmdline_state (state, *rd, &use);

	  /* If a variable was being processed and this character does
	     not describe the variable anymore, write the variable to
	     the buffer.  */
	  add_var (newstate);

	  if (check_varstate (newstate))
	    {
	      if (use)
		*(vp++) = use;
	    }
	  else
	    {
	      if (newstate == GRUB_PARSER_STATE_TEXT
		  && state != GRUB_PARSER_STATE_ESC && grub_isspace (use))
		{
		  /* Don't add more than one argument if multiple
		     spaces are used.  */
		  if (bp != buffer && *(bp - 1))
		    {
		      *(bp++) = '\0';
		      (*argc)++;
		    }
		}
	      else if (use)
		*(bp++) = use;
	    }
	  state = newstate;
	}
    }
  while (state != GRUB_PARSER_STATE_TEXT && !check_varstate (state));

  /* A special case for when the last character was part of a
     variable.  */
  add_var (GRUB_PARSER_STATE_TEXT);

  if (bp != buffer && *(bp - 1))
    {
      *(bp++) = '\0';
      (*argc)++;
    }

  /* Reserve memory for the return values.  */
  args = grub_malloc (bp - buffer);
  if (!args)
    return grub_errno;
  grub_memcpy (args, buffer, bp - buffer);

  *argv = grub_malloc (sizeof (char *) * (*argc + 1));
  if (!*argv)
    {
      grub_free (args);
      return grub_errno;
    }

  /* The arguments are separated with 0's, setup argv so it points to
     the right values.  */
  bp = args;
  for (i = 0; i < *argc; i++)
    {
      (*argv)[i] = bp;
      while (*bp)
	bp++;
      bp++;
    }

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
* @date 注释添加日期：2013年6月21日
*
* @brief 解析并执行多行命令输入。
*
* @note 注释详细内容:
*
* 本函数实现解析并执行多行命令输入的功能。函数循环调用内部的getline()函数分离出以'\n'分
* 开的每一行，然后使用grub_rescue_parse_line()进一步对输入行进行解析并执行，直到将source
* 输入的每一行都解析执行完。
**/
grub_err_t
grub_parser_execute (char *source)
{
  auto grub_err_t getline (char **line, int cont);
  grub_err_t getline (char **line, int cont __attribute__ ((unused)))
  {
    char *p;

    if (!source)
      {
	*line = 0;
	return 0;
      }

    p = grub_strchr (source, '\n');

    if (p)
      *line = grub_strndup (source, p - source);
    else
      *line = grub_strdup (source);
    source = p ? p + 1 : 0;
    return 0;
  }

  while (source)
    {
      char *line;

      getline (&line, 0);
      grub_rescue_parse_line (line, getline);
      grub_free (line);
    }

  return grub_errno;
}
