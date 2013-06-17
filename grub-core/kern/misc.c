/* misc.c - definitions of misc functions */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#include <grub/misc.h>
#include <grub/err.h>
#include <grub/mm.h>
#include <stdarg.h>
#include <grub/term.h>
#include <grub/env.h>
#include <grub/i18n.h>

static int
grub_vsnprintf_real (char *str, grub_size_t n, const char *fmt, va_list args);
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
* @brief 判断一个字符是不是单词的分隔符。
*
* @note 注释详细内容: 
*
* 本函数实现判断一个字符是不是单词的分隔符的功能。如果该字符是空格" "，逗号",",
* 竖直"|"，或者C语言的取地址符号"&"，那么该函数返回非零值。
**/

static int
grub_iswordseparator (int c)
{
  return (grub_isspace (c) || c == ',' || c == ';' || c == '|' || c == '&');
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
* @brief 翻译字符串函数指针。
*
* @note 注释详细内容: 
*
* 本函数实现翻译字符串的功能。gettext的工作流程是这样的：比如我们写一个C程序，
* 通常printf等输出信息都是English的。如果我们在程序中加入gettext支持，在需要交 
* 互的字符串上用gettext函数，程序运行是就可以先调用gettext函数处理字符串，替换
* 当前的字符串了。注意是运行时替换。(本函数实现实际上不做任何翻译)。
**/

/* grub_gettext_dummy is not translating anything.  */
static const char *
grub_gettext_dummy (const char *s)
{
  return s;
}

const char* (*grub_gettext) (const char *s) = grub_gettext_dummy;

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
* @brief 由src所指内存区域复制n个字节到dest所指内存区域。
*
* @note 注释详细内容: 
*
* memmove()与memcpy()一样都是用来拷贝src所指的内存内容前n个字节到dest所指的地址
* 上。不同的是,当src和dest所指的内存区域重叠时,memmove()仍然可以正确的处理,不过
* 执行效率上会比使用memcpy()略慢些。返回指向dest的指针。
*
* memcpy()、 memmove()和memccpy() 这三个函数的功能均是将某个内存块复制到另一个
* 内存块。前两个函数的区别在于它们处理内存区域重叠(overlapping)的方式不同。第三
* 个函数的功能也是复制内存，但是如果遇到某个特定值时立即停止复制。
*
* 对于库函数来说，由于没有办法知道传递给他的内存区域的情况，所以应该使用memmove()
* 函数。通过这个函数，可以保证不会出现任何内存块重叠问题。而对于应用程序来说，
* 因为代码“知道”两个内存块不会重叠，所以可以安全地使用memcpy()函数。
**/

void *
grub_memmove (void *dest, const void *src, grub_size_t n)
{
  char *d = (char *) dest;
  const char *s = (const char *) src;

  if (d < s)
    while (n--)
      *d++ = *s++;
  else
    {
      d += n;
      s += n;

      while (n--)
	*--d = *--s;
    }

  return dest;
}

#ifndef __APPLE__
void *memmove (void *dest, const void *src, grub_size_t n)
  __attribute__ ((alias ("grub_memmove")));
/* GCC emits references to memcpy() for struct copies etc.  */
void *memcpy (void *dest, const void *src, grub_size_t n)
  __attribute__ ((alias ("grub_memmove")));
#else
void * __attribute__ ((regparm(0)))
memcpy (void *dest, const void *src, grub_size_t n)
{
	return grub_memmove (dest, src, n);
}
void * __attribute__ ((regparm(0)))
memmove (void *dest, const void *src, grub_size_t n)
{
	return grub_memmove (dest, src, n);
}
#endif

char *
grub_strcpy (char *dest, const char *src)
{
  char *p = dest;

  while ((*p++ = *src++) != '\0')
    ;

  return dest;
}

char *
grub_strncpy (char *dest, const char *src, int c)
{
  char *p = dest;

  while ((*p++ = *src++) != '\0' && --c)
    ;

  return dest;
}

int
grub_printf (const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start (ap, fmt);
  ret = grub_vprintf (fmt, ap);
  va_end (ap);

  return ret;
}

int
grub_printf_ (const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start (ap, fmt);
  ret = grub_vprintf (_(fmt), ap);
  va_end (ap);

  return ret;
}

int
grub_puts_ (const char *s)
{
  return grub_puts (_(s));
}

#if defined (__APPLE__) && ! defined (GRUB_UTIL)
int
grub_err_printf (const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start (ap, fmt);
	ret = grub_vprintf (fmt, ap);
	va_end (ap);

	return ret;
}
#endif

#if ! defined (__APPLE__) && ! defined (GRUB_UTIL)
int grub_err_printf (const char *fmt, ...)
__attribute__ ((alias("grub_printf")));
#endif

void
grub_real_dprintf (const char *file, const int line, const char *condition,
		   const char *fmt, ...)
{
  va_list args;
  const char *debug = grub_env_get ("debug");

  if (! debug)
    return;

  if (grub_strword (debug, "all") || grub_strword (debug, condition))
    {
      grub_printf ("%s:%d: ", file, line);
      va_start (args, fmt);
      grub_vprintf (fmt, args);
      va_end (args);
      grub_refresh ();
    }
}

#define PREALLOC_SIZE 255

int
grub_vprintf (const char *fmt, va_list args)
{
  grub_size_t s;
  static char buf[PREALLOC_SIZE + 1];
  char *curbuf = buf;
  va_list ap2;
  va_copy (ap2, args);

  s = grub_vsnprintf_real (buf, PREALLOC_SIZE, fmt, args);
  if (s > PREALLOC_SIZE)
    {
      curbuf = grub_malloc (s + 1);
      if (!curbuf)
	{
	  grub_errno = GRUB_ERR_NONE;
	  buf[PREALLOC_SIZE - 3] = '.';
	  buf[PREALLOC_SIZE - 2] = '.';
	  buf[PREALLOC_SIZE - 1] = '.';
	  buf[PREALLOC_SIZE] = 0;
	  curbuf = buf;
	}
      else
	s = grub_vsnprintf_real (curbuf, s, fmt, ap2);
    }

  va_end (ap2);

  grub_xputs (curbuf);

  if (curbuf != buf)
    grub_free (curbuf);
  
  return s;
}

int
grub_memcmp (const void *s1, const void *s2, grub_size_t n)
{
  const grub_uint8_t *t1 = s1;
  const grub_uint8_t *t2 = s2;

  while (n--)
    {
      if (*t1 != *t2)
	return (int) *t1 - (int) *t2;

      t1++;
      t2++;
    }

  return 0;
}
#ifndef __APPLE__
int memcmp (const void *s1, const void *s2, grub_size_t n)
  __attribute__ ((alias ("grub_memcmp")));
#else
int __attribute__ ((regparm(0)))
memcmp (const void *s1, const void *s2, grub_size_t n)
{
  return grub_memcmp (s1, s2, n);
}
#endif

int
grub_strcmp (const char *s1, const char *s2)
{
  while (*s1 && *s2)
    {
      if (*s1 != *s2)
	break;

      s1++;
      s2++;
    }

  return (int) (grub_uint8_t) *s1 - (int) (grub_uint8_t) *s2;
}

int
grub_strncmp (const char *s1, const char *s2, grub_size_t n)
{
  if (n == 0)
    return 0;

  while (*s1 && *s2 && --n)
    {
      if (*s1 != *s2)
	break;

      s1++;
      s2++;
    }

  return (int) (grub_uint8_t) *s1 - (int) (grub_uint8_t)  *s2;
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
* @brief 查找字符串s中首次出现字符c的位置。
*
* @note 注释详细内容: 
*
* 本函数实现兼容C语言函数库的strchr的功能。查找字符串s中首次出现字符c的位置。
* 返回首次出现c的位置的指针，如果s中不存在c则返回NULL。
**/

char *
grub_strchr (const char *s, int c)
{
  do
    {
      if (*s == c)
	return (char *) s;
    }
  while (*s++);

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
* @brief 查找一个字符c在另一个字符串s中末次出现的位置。
*
* @note 注释详细内容: 
*
* 本函数实现兼容C语言函数库的strrchr的功能。查找一个字符c在另一个字符串s中末次
* 出现的位置（也就是从s的右侧开始查找字符c首次出现的位置），并返回从字符串中的
* 这个位置起，一直到字符串结束的所有字符。如果未能找到指定字符，那么函数将返回
* NULL。
**/

char *
grub_strrchr (const char *s, int c)
{
  char *p = NULL;

  do
    {
      if (*s == c)
	p = (char *) s;
    }
  while (*s++);

  return p;
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
* @brief 在一个字符串haystack中查找一个单词needle。
*
* @note 注释详细内容: 
*
* 本函数实现在一个字符串haystack中查找一个单词needle的功能。找到返回1，否则返回0。
**/

int
grub_strword (const char *haystack, const char *needle)
{
  const char *n_pos = needle;

  while (grub_iswordseparator (*haystack))
    haystack++;

  while (*haystack)
    {
      /* Crawl both the needle and the haystack word we're on.  */
      while(*haystack && !grub_iswordseparator (*haystack)
            && *haystack == *n_pos)
        {
          haystack++;
          n_pos++;
        }

      /* If we reached the end of both words at the same time, the word
      is found. If not, eat everything in the haystack that isn't the
      next word (or the end of string) and "reset" the needle.  */
      if ( (!*haystack || grub_iswordseparator (*haystack))
         && (!*n_pos || grub_iswordseparator (*n_pos)))
        return 1;
      else
        {
          n_pos = needle;
          while (*haystack && !grub_iswordseparator (*haystack))
            haystack++;
          while (grub_iswordseparator (*haystack))
            haystack++;
        }
    }

  return 0;
}

int
grub_isspace (int c)
{
  return (c == '\n' || c == '\r' || c == ' ' || c == '\t');
}

int
grub_isprint (int c)
{
  return (c >= ' ' && c <= '~');
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
* @brief 将字符串转换成无符号长整型数，返回转换后的长整型数。
*
* @note 注释详细内容: 
*
* 本函数实现将字符串转换成无符号长整型数的功能。strtoul()会将参数str字符串根据
* 参数base来转换成无符号的长整型数。参数base范围从2至36，或0。参数base代表采用
* 的进制方式，如base值为10则采用10进制，若base值为16则采用16进制数等。当base值
* 为0时会根据情况选择用哪种进制：如果第一个字符是'0'，就判断第二字符如果是‘x’
* 则用16进制，否则用8进制；第一个字符不是‘0’，则用10进制。一开始strtoul()会
* 扫描参数str字符串，跳过前面的空格字符串，直到遇上数字或正负符号才开始做转换，
* 再遇到非数字或字符串结束时('')结束转换，并将结果返回。若参数end不为NULL，
* 则会将遇到不合条件而终止的str中的字符指针由end返回。
**/

unsigned long
grub_strtoul (const char *str, char **end, int base)
{
  unsigned long long num;

  num = grub_strtoull (str, end, base);
#if GRUB_CPU_SIZEOF_LONG != 8
  if (num > ~0UL)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE, N_("overflow is detected"));
      return ~0UL;
    }
#endif

  return (unsigned long) num;
}

unsigned long long
grub_strtoull (const char *str, char **end, int base)
{
  unsigned long long num = 0;
  int found = 0;

  /* Skip white spaces.  */
  while (*str && grub_isspace (*str))
    str++;

  /* Guess the base, if not specified. The prefix `0x' means 16, and
     the prefix `0' means 8.  */
  if (str[0] == '0')
    {
      if (str[1] == 'x')
	{
	  if (base == 0 || base == 16)
	    {
	      base = 16;
	      str += 2;
	    }
	}
      else if (base == 0 && str[1] >= '0' && str[1] <= '7')
	base = 8;
    }

  if (base == 0)
    base = 10;

  while (*str)
    {
      unsigned long digit;

      digit = grub_tolower (*str) - '0';
      if (digit > 9)
	{
	  digit += '0' - 'a' + 10;
	  if (digit >= (unsigned long) base)
	    break;
	}

      found = 1;

      /* NUM * BASE + DIGIT > ~0ULL */
      if (num > grub_divmod64 (~0ULL - digit, base, 0))
	{
	  grub_error (GRUB_ERR_OUT_OF_RANGE,
		      N_("overflow is detected"));
	  return ~0ULL;
	}

      num = num * base + digit;
      str++;
    }

  if (! found)
    {
      grub_error (GRUB_ERR_BAD_NUMBER,
		  N_("unrecognized number"));
      return 0;
    }

  if (end)
    *end = (char *) str;

  return num;
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
* @brief 将参数 s 指向的字符串复制到一个字符串指针上去。
*
* @note 注释详细内容: 
*
* 本函数实现兼容C语言函数库的strdup的功能。
*
* grub_strdup() 函数将参数 s 指向的字符串复制到一个字符串指针上去，这个字符串指针
* 事先可以没被初始化。在复制时，grub_strdup() 会给这个指针使用 grub_malloc() 分
* 配空间，如果不再使用这个指针，相应的用 grub_free() 来释放掉这部分空间。
*
* strndup() 函数只复制前面 n 个字符。
**/
char *
grub_strdup (const char *s)
{
  grub_size_t len;
  char *p;

  len = grub_strlen (s) + 1;
  p = (char *) grub_malloc (len);
  if (! p)
    return 0;

  return grub_memcpy (p, s, len);
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
* @brief 将参数 s 指向的字符串的前面 n 个字符复制到一个字符串指针上去。
*
* @note 注释详细内容: 
*
* 本函数实现兼容C语言函数库的strdup的功能。
*
* grub_strdup() 函数将参数 s 指向的字符串复制到一个字符串指针上去，这个字符串指针
* 事先可以没被初始化。在复制时，grub_strdup() 会给这个指针使用 grub_malloc() 分
* 配空间，如果不再使用这个指针，相应的用 grub_free() 来释放掉这部分空间。
*
* strndup() 函数只复制前面 n 个字符。
**/

char *
grub_strndup (const char *s, grub_size_t n)
{
  grub_size_t len;
  char *p;

  len = grub_strlen (s);
  if (len > n)
    len = n;
  p = (char *) grub_malloc (len + 1);
  if (! p)
    return 0;

  grub_memcpy (p, s, len);
  p[len] = '\0';
  return p;
}

void *
grub_memset (void *s, int c, grub_size_t len)
{
  void *p = s;
  grub_uint8_t pattern8 = c;

  if (len >= 3 * sizeof (unsigned long))
    {
      unsigned long patternl = 0;
      grub_size_t i;

      for (i = 0; i < sizeof (unsigned long); i++)
	patternl |= ((unsigned long) pattern8) << (8 * i);

      while (len > 0 && (((grub_addr_t) p) & (sizeof (unsigned long) - 1)))
	{
	  *(grub_uint8_t *) p = pattern8;
	  p = (grub_uint8_t *) p + 1;
	  len--;
	}
      while (len >= sizeof (unsigned long))
	{
	  *(unsigned long *) p = patternl;
	  p = (unsigned long *) p + 1;
	  len -= sizeof (unsigned long);
	}
    }

  while (len > 0)
    {
      *(grub_uint8_t *) p = pattern8;
      p = (grub_uint8_t *) p + 1;
      len--;
    }

  return s;
}
#ifndef __APPLE__
void *memset (void *s, int c, grub_size_t n)
  __attribute__ ((alias ("grub_memset")));
#else
void * __attribute__ ((regparm(0)))
memset (void *s, int c, grub_size_t n)
{
  return grub_memset (s, c, n);
}
#endif

grub_size_t
grub_strlen (const char *s)
{
  const char *p = s;

  while (*p)
    p++;

  return p - s;
}

static inline void
grub_reverse (char *str)
{
  char *p = str + grub_strlen (str) - 1;

  while (str < p)
    {
      char tmp;

      tmp = *str;
      *str = *p;
      *p = tmp;
      str++;
      p--;
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
* @brief 完成64位除法运算。
*
* @note 注释详细内容: 
*
* 本函数实现完成64位除法运算的功能。将64位数n用64位数d除，将除法的商作为返回值，
* 将余数存储在输出参数r中。
**/

/* Divide N by D, return the quotient, and store the remainder in *R.  */
grub_uint64_t
grub_divmod64 (grub_uint64_t n, grub_uint64_t d, grub_uint64_t *r)
{
  /* This algorithm is typically implemented by hardware. The idea
     is to get the highest bit in N, 64 times, by keeping
     upper(N * 2^i) = (Q * D + M), where upper
     represents the high 64 bits in 128-bits space.  */
  unsigned bits = 64;
  grub_uint64_t q = 0;
  grub_uint64_t m = 0;

  /* Skip the slow computation if 32-bit arithmetic is possible.  */
  if (n < 0xffffffff && d < 0xffffffff)
    {
      if (r)
	*r = ((grub_uint32_t) n) % (grub_uint32_t) d;

      return ((grub_uint32_t) n) / (grub_uint32_t) d;
    }

  while (bits--)
    {
      m <<= 1;

      if (n & (1ULL << 63))
	m |= 1;

      q <<= 1;
      n <<= 1;

      if (m >= d)
	{
	  q |= 1;
	  m -= d;
	}
    }

  if (r)
    *r = m;

  return q;
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
* @brief 将long long类型的参数n转换为字符串。
*
* @note 注释详细内容: 
*
* 本函数实现将long long类型的参数n转换为字符串的功能。参数str为转换后存储到的
* 目的字符串，参数c为转换的基数，参数n为要转换的64位无符号数(unsigned long long)。
**/

/* Convert a long long value to a string. This function avoids 64-bit
   modular arithmetic or divisions.  */
static char *
grub_lltoa (char *str, int c, unsigned long long n)
{
  unsigned base = (c == 'x') ? 16 : 10;
  char *p;

  if ((long long) n < 0 && c == 'd')
    {
      n = (unsigned long long) (-((long long) n));
      *str++ = '-';
    }

  p = str;

  if (base == 16)
    do
      {
	unsigned d = (unsigned) (n & 0xf);
	*p++ = (d > 9) ? d + 'a' - 10 : d + '0';
      }
    while (n >>= 4);
  else
    /* BASE == 10 */
    do
      {
	grub_uint64_t m;

	n = grub_divmod64 (n, 10, &m);
	*p++ = m + '0';
      }
    while (n);

  *p = 0;

  grub_reverse (str);
  return p;
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
* @brief 将可变参数格式化输出到一个字符串。
*
* @note 注释详细内容: 
*
* 本函数实现将可变参数格式化输出到一个字符串的功能。参数str为把生成的格式化的字
* 符串存放在这里;参数max_len控制buffer可接受的最大字节数,防止产生数组越界；参数
* fmt0为指定输出格式的字符串，它决定了你需要提供的可变参数的类型、个数和顺序；
* 参数args_in为可变参数。用法类似于vsprintf，不过加了max_len的限制，防止了内存溢出
*（max_len为str所指的存储空间的大小）。执行成功，返回写入到字符数组str中的字符个数
*（不包含终止符），最大不超过max_len；执行失败，返回负值。
**/

static int
grub_vsnprintf_real (char *str, grub_size_t max_len, const char *fmt0, va_list args_in)
{
  char c;
  grub_size_t n = 0;
  grub_size_t count = 0;
  grub_size_t count_args = 0;
  const char *fmt;
  auto void write_char (unsigned char ch);
  auto void write_str (const char *s);
  auto void write_fill (const char ch, int n);

  void write_char (unsigned char ch)
    {
      if (count < max_len)
	*str++ = ch;

      count++;
    }

  void write_str (const char *s)
    {
      while (*s)
	write_char (*s++);
    }

  void write_fill (const char ch, int count_fill)
    {
      int i;
      for (i = 0; i < count_fill; i++)
	write_char (ch);
    }

  fmt = fmt0;
  while ((c = *fmt++) != 0)
    {
      if (c != '%')
	continue;

      if (*fmt && *fmt =='-')
	fmt++;

      while (*fmt && grub_isdigit (*fmt))
	fmt++;

      if (*fmt && *fmt == '$')
	fmt++;

      if (*fmt && *fmt =='-')
	fmt++;

      while (*fmt && grub_isdigit (*fmt))
	fmt++;

      if (*fmt && *fmt =='.')
	fmt++;

      while (*fmt && grub_isdigit (*fmt))
	fmt++;

      c = *fmt++;
      if (c == 'l')
	{
	  c = *fmt++;
	  if (c == 'l')
	    c = *fmt++;
	}
      switch (c)
	{
	case 'p':
	case 'x':
	case 'u':
	case 'd':
	case 'c':
	case 'C':
	case 's':
	  count_args++;
	  break;
	}
    }

  enum { INT, WCHAR, LONG, LONGLONG, POINTER } types[count_args];
  union 
  { 
    int i;
    grub_uint32_t w;
    long l;
    long long ll;
    void *p;
  } args[count_args];

  grub_memset (types, 0, sizeof (types));

  fmt = fmt0;
  n = 0;
  while ((c = *fmt++) != 0)
    {
      int longfmt = 0;
      int longlongfmt = 0;
      grub_size_t curn;
      const char *p;

      if (c != '%')
	continue;

      curn = n++;

      if (*fmt && *fmt =='-')
	fmt++;

      while (*fmt && grub_isdigit (*fmt))
	fmt++;

      if (*fmt && *fmt =='.')
	fmt++;

      while (*fmt && grub_isdigit (*fmt))
	fmt++;

      p = fmt;

      if (*fmt && *fmt == '$')
	{
	  curn = grub_strtoull (p, 0, 10) - 1;
	  fmt++;
	}

      while (*fmt && grub_isdigit (*fmt))
	fmt++;

      c = *fmt++;
      if (c == 'l')
	{
	  c = *fmt++;
	  longfmt = 1;
	  if (c == 'l')
	    {
	      c = *fmt++;
	      longlongfmt = 1;
	    }
	}
      if (curn >= count_args)
	continue;
      switch (c)
	{
	case 'x':
	case 'u':
	case 'd':
	  if (longlongfmt)
	    types[curn] = LONGLONG;
	  else if (longfmt)
	    types[curn] = LONG;
	  else
	    types[curn] = INT;
	  break;
	case 'p':
	case 's':
	  types[curn] = POINTER;
	  break;
	case 'c':
	  types[curn] = INT;
	  break;
	case 'C':
	  types[curn] = WCHAR;
	  break;
	}
    }

  for (n = 0; n < count_args; n++)
    switch (types[n])
      {
      case WCHAR:
	args[n].w = va_arg (args_in, grub_uint32_t);
	break;
      case POINTER:
	args[n].p = va_arg (args_in, void *);
	break;
      case INT:
	args[n].i = va_arg (args_in, int);
	break;
      case LONG:
	args[n].l = va_arg (args_in, long);
	break;
      case LONGLONG:
	args[n].ll = va_arg (args_in, long long);
	break;
      }

  fmt = fmt0;

  n = 0;
  while ((c = *fmt++) != 0)
    {
      char tmp[32];
      char *p;
      unsigned int format1 = 0;
      unsigned int format2 = ~ 0U;
      char zerofill = ' ';
      int rightfill = 0;
      int longfmt = 0;
      int longlongfmt = 0;
      int unsig = 0;
      grub_size_t curn;
      
      if (c != '%')
	{
	  write_char (c);
	  continue;
	}

      curn = n++;

    rescan:;

      if (*fmt && *fmt =='-')
	{
	  rightfill = 1;
	  fmt++;
	}

      p = (char *) fmt;
      /* Read formatting parameters.  */
      while (*p && grub_isdigit (*p))
	p++;

      if (p > fmt)
	{
	  char s[p - fmt + 1];
	  grub_strncpy (s, fmt, p - fmt);
	  s[p - fmt] = 0;
	  if (s[0] == '0')
	    zerofill = '0';
	  format1 = grub_strtoul (s, 0, 10);
	  fmt = p;
	}

      if (*p && *p == '.')
	{
	  p++;
	  fmt++;
	  while (*p && grub_isdigit (*p))
	    p++;

	  if (p > fmt)
	    {
	      char fstr[p - fmt + 1];
	      grub_strncpy (fstr, fmt, p - fmt);
	      fstr[p - fmt] = 0;
	      format2 = grub_strtoul (fstr, 0, 10);
	      fmt = p;
	    }
	}
      if (*fmt == '$')
	{
	  curn = format1 - 1;
	  fmt++;
	  format1 = 0;
	  format2 = ~ 0U;
	  zerofill = ' ';
	  rightfill = 0;

	  goto rescan;
	}

      c = *fmt++;
      if (c == 'l')
	{
	  longfmt = 1;
	  c = *fmt++;
	  if (c == 'l')
	    {
	      longlongfmt = 1;
	      c = *fmt++;
	    }
	}

      if (curn >= count_args)
	continue;

      switch (c)
	{
	case 'p':
	  write_str ("0x");
	  c = 'x';
	  longlongfmt |= (sizeof (void *) == sizeof (long long));
	  /* Fall through. */
	case 'x':
	case 'u':
	  unsig = 1;
	  /* Fall through. */
	case 'd':
	  if (longlongfmt)
	    grub_lltoa (tmp, c, args[curn].ll);
	  else if (longfmt && unsig)
	    grub_lltoa (tmp, c, (unsigned long) args[curn].l);
	  else if (longfmt)
	    grub_lltoa (tmp, c, args[curn].l);
	  else if (unsig)
	    grub_lltoa (tmp, c, (unsigned) args[curn].i);
	  else
	    grub_lltoa (tmp, c, args[curn].i);
	  if (! rightfill && grub_strlen (tmp) < format1)
	    write_fill (zerofill, format1 - grub_strlen (tmp));
	  write_str (tmp);
	  if (rightfill && grub_strlen (tmp) < format1)
	    write_fill (zerofill, format1 - grub_strlen (tmp));
	  break;

	case 'c':
	  write_char (args[curn].i & 0xff);
	  break;

	case 'C':
	  {
	    grub_uint32_t code = args[curn].w;
	    int shift;
	    unsigned mask;

	    if (code <= 0x7f)
	      {
		shift = 0;
		mask = 0;
	      }
	    else if (code <= 0x7ff)
	      {
		shift = 6;
		mask = 0xc0;
	      }
	    else if (code <= 0xffff)
	      {
		shift = 12;
		mask = 0xe0;
	      }
	    else if (code <= 0x1fffff)
	      {
		shift = 18;
		mask = 0xf0;
	      }
	    else if (code <= 0x3ffffff)
	      {
		shift = 24;
		mask = 0xf8;
	      }
	    else if (code <= 0x7fffffff)
	      {
		shift = 30;
		mask = 0xfc;
	      }
	    else
	      {
		code = '?';
		shift = 0;
		mask = 0;
	      }

	    write_char (mask | (code >> shift));

	    for (shift -= 6; shift >= 0; shift -= 6)
	      write_char (0x80 | (0x3f & (code >> shift)));
	  }
	  break;

	case 's':
	  p = args[curn].p;
	  if (p)
	    {
	      grub_size_t len = 0;
	      while (len < format2 && p[len])
		len++;

	      if (!rightfill && len < format1)
		write_fill (zerofill, format1 - len);

	      grub_size_t i;
	      for (i = 0; i < len; i++)
		write_char (*p++);

	      if (rightfill && len < format1)
		write_fill (zerofill, format1 - len);
	    }
	  else
	    write_str ("(null)");

	  break;

	default:
	  write_char (c);
	  break;
	}
    }

  *str = '\0';

  return count;
}

int
grub_vsnprintf (char *str, grub_size_t n, const char *fmt, va_list ap)
{
  grub_size_t ret;

  if (!n)
    return 0;

  n--;

  ret = grub_vsnprintf_real (str, n, fmt, ap);

  return ret < n ? ret : n;
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
* @brief 将可变个参数(...)按照fmt格式化成字符串，然后将其复制到str中。
*
* @note 注释详细内容: 
*
* 本函数实现将可变个参数(...)按照fmt格式化成字符串，然后将其复制到str中的功能。
* (1) 如果格式化后的字符串长度 < n，则将此字符串全部复制到str中，并给其后添加
* 一个字符串结束符('\0')；(2) 如果格式化后的字符串长度 >= n，则只将其中的(n-1)
* 个字符复制到str中，并给其后添加一个字符串结束符('\0')，返回值为格式化后的字
* 符串的长度。
**/

int
grub_snprintf (char *str, grub_size_t n, const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start (ap, fmt);
  ret = grub_vsnprintf (str, n, fmt, ap);
  va_end (ap);

  return ret;
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
* @brief 将可变个参数ap按照fmt格式化成字符串并返回。
*
* @note 注释详细内容: 
*
* 本函数实现将可变个参数ap按照fmt格式化成字符串并返回的功能。
*
* (1) 如果格式化后的字符串长度 < PREALLOC_SIZE，则将此字符串全部复制到str中，并给其后添加
* 一个字符串结束符('\0')；(2) 如果格式化后的字符串长度 >= PREALLOC_SIZE，则只将其中的(n-1)
* 个字符复制到str中，并给其后添加一个字符串结束符('\0')，返回值为格式化后的字
* 符串的长度。
**/
char *
grub_xvasprintf (const char *fmt, va_list ap)
{
  grub_size_t s, as = PREALLOC_SIZE;
  char *ret;

  while (1)
    {
      va_list ap2;
      ret = grub_malloc (as + 1);
      if (!ret)
	return NULL;

      va_copy (ap2, ap);

      s = grub_vsnprintf_real (ret, as, fmt, ap2);

      va_end (ap2);

      if (s <= as)
	return ret;

      grub_free (ret);
      as = s;
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
* @brief 将可变个参数ap按照fmt格式化成字符串并返回。
*
* @note 注释详细内容: 
*
* 本函数实现将可变个参数(...)按照fmt格式化成字符串并返回的功能。
*
* (1) 如果格式化后的字符串长度 < PREALLOC_SIZE，则将此字符串全部复制到str中，并给其后添加
* 一个字符串结束符('\0')；(2) 如果格式化后的字符串长度 >= PREALLOC_SIZE，则只将其中的(n-1)
* 个字符复制到str中，并给其后添加一个字符串结束符('\0')，返回值为格式化后的字
* 符串的长度。
**/
char *
grub_xasprintf (const char *fmt, ...)
{
  va_list ap;
  char *ret;

  va_start (ap, fmt);
  ret = grub_xvasprintf (fmt, ap);
  va_end (ap);

  return ret;
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
* @brief 中止运行当前命令。
*
* @note 注释详细内容: 
*
* 本函数实现中止运行当前命令的功能。
**/
/* Abort GRUB. This function does not return.  */
void
grub_abort (void)
{
  grub_printf ("\nAborted.");
  
#ifndef GRUB_UTIL
  if (grub_term_inputs)
#endif
    {
      grub_printf (" Press any key to exit.");
      grub_getkey ();
    }

  grub_exit ();
}

#if ! defined (__APPLE__) && !defined (GRUB_UTIL)
/* GCC emits references to abort().  */
void abort (void) __attribute__ ((alias ("grub_abort")));
#endif

#if NEED_ENABLE_EXECUTE_STACK && !defined(GRUB_UTIL) && !defined(GRUB_MACHINE_EMU)
/* Some gcc versions generate a call to this function
   in trampolines for nested functions.  */
void __enable_execute_stack (void *addr __attribute__ ((unused)))
{
}
#endif

#if NEED_REGISTER_FRAME_INFO && !defined(GRUB_UTIL)
void __register_frame_info (void)
{
}

void __deregister_frame_info (void)
{
}
#endif

