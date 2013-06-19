/* dl.c - loadable module support */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004,2005,2007,2008,2009  Free Software Foundation, Inc.
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

/* Force native word size */
#define GRUB_TARGET_WORDSIZE (8 * GRUB_CPU_SIZEOF_VOID_P)

#include <config.h>
#include <grub/elf.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/types.h>
#include <grub/symbol.h>
#include <grub/file.h>
#include <grub/env.h>
#include <grub/cache.h>
#include <grub/i18n.h>

/* Platforms where modules are in a readonly area of memory.  */
#if defined(GRUB_MACHINE_QEMU)
#define GRUB_MODULES_MACHINE_READONLY
#endif

#ifdef GRUB_MACHINE_EMU
#include <sys/mman.h>
#endif



#pragma GCC diagnostic ignored "-Wcast-align"

grub_dl_t grub_dl_head = 0;

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
* @brief 添加一个可加载模块到全局模块列表中。
*
* @note 注释详细内容:
*
* 本函数实现添加一个可加载模块到全局模块列表中的功能。将这个模块添加到grub_dl_head的首项。
**/
grub_err_t
grub_dl_add (grub_dl_t mod);

grub_err_t
grub_dl_add (grub_dl_t mod)
{
  if (grub_dl_get (mod->name))
    return grub_error (GRUB_ERR_BAD_MODULE,
		       "`%s' is already loaded", mod->name);

  mod->next = grub_dl_head;
  grub_dl_head = mod;

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
* @brief 从全局模块列表中删除一个已经添加的模块。
*
* @note 注释详细内容:
*
* 本函数实现从全局模块列表中删除一个已经添加的模块的功能。
**/
static void
grub_dl_remove (grub_dl_t mod)
{
  grub_dl_t *p, q;

  for (p = &grub_dl_head, q = *p; q; p = &q->next, q = *p)
    if (q == mod)
      {
	*p = q->next;
	return;
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
* @brief 在全局模块列表中按照模块名字查询并获取模块指针。
*
* @note 注释详细内容:
*
* 本函数实现在全局模块列表中按照模块名字查询并获取模块指针的功能。
**/
grub_dl_t
grub_dl_get (const char *name)
{
  grub_dl_t l;

  for (l = grub_dl_head; l; l = l->next)
    if (grub_strcmp (name, l->name) == 0)
      return l;

  return 0;
}

struct grub_symbol
{
  struct grub_symbol *next;
  const char *name;
  void *addr;
  int isfunc;
  grub_dl_t mod;	/* The module to which this symbol belongs.  */
};
typedef struct grub_symbol *grub_symbol_t;

/* The size of the symbol table.  */
#define GRUB_SYMTAB_SIZE	509

/* The symbol table (using an open-hash).  */
static struct grub_symbol *grub_symtab[GRUB_SYMTAB_SIZE];

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
* @brief 计算字符串的hash键值。
*
* @note 注释详细内容:
*
* 本函数实现计算字符串的hash键值的功能。
**/
/* Simple hash function.  */
static unsigned
grub_symbol_hash (const char *s)
{
  unsigned key = 0;

  while (*s)
    key = key * 65599 + *s++;

  return (key + (key >> 5)) % GRUB_SYMTAB_SIZE;
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
* @brief 按符号的名字查找符号。
*
* @note 注释详细内容:
*
* 本函数实现按符号的名字查找符号的功能。通过grub_symbol_hash()先找到这个名字的符号在符
* 号表grub_symtab中的索引，然后按名字比较，在该索引的符号链表中查找对应的符号结构并返回。
**/
/* Resolve the symbol name NAME and return the address.
   Return NULL, if not found.  */
static grub_symbol_t
grub_dl_resolve_symbol (const char *name)
{
  grub_symbol_t sym;

  for (sym = grub_symtab[grub_symbol_hash (name)]; sym; sym = sym->next)
    if (grub_strcmp (sym->name, name) == 0)
      return sym;

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
* @brief 按符号的名字注册符号。
*
* @note 注释详细内容:
*
* 本函数实现按符号的名字注册符号的功能。通过grub_symbol_hash()先找到这个名字的符号在符
* 号表grub_symtab中的索引，然后将对应的符号结构插入到对应的符号链表的首项。
**/
/* Register a symbol with the name NAME and the address ADDR.  */
grub_err_t
grub_dl_register_symbol (const char *name, void *addr, int isfunc,
			 grub_dl_t mod)
{
  grub_symbol_t sym;
  unsigned k;

  sym = (grub_symbol_t) grub_malloc (sizeof (*sym));
  if (! sym)
    return grub_errno;

  if (mod)
    {
      sym->name = grub_strdup (name);
      if (! sym->name)
	{
	  grub_free (sym);
	  return grub_errno;
	}
    }
  else
    sym->name = name;

  sym->addr = addr;
  sym->mod = mod;
  sym->isfunc = isfunc;

  k = grub_symbol_hash (name);
  sym->next = grub_symtab[k];
  grub_symtab[k] = sym;

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
* @brief 取消注册一个模块的所有符号。
*
* @note 注释详细内容:
*
* 本函数实现取消注册一个模块的所有符号的功能。扫描grub_symtab的每一个索引，每一个上面对
* 应的符号链表都比较，如果该符号对应的模块是参数mod，那么就将该符号从链表删除并释放该符
* 号对应的符号资源。
**/
/* Unregister all the symbols defined in the module MOD.  */
static void
grub_dl_unregister_symbols (grub_dl_t mod)
{
  unsigned i;

  if (! mod)
    grub_fatal ("core symbols cannot be unregistered");

  for (i = 0; i < GRUB_SYMTAB_SIZE; i++)
    {
      grub_symbol_t sym, *p, q;

      for (p = &grub_symtab[i], sym = *p; sym; sym = q)
	{
	  q = sym->next;
	  if (sym->mod == mod)
	    {
	      *p = q;
	      grub_free ((void *) sym->name);
	      grub_free (sym);
	    }
	  else
	    p = &sym->next;
	}
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
* @brief 获得模块的指定索引的section地址。
*
* @note 注释详细内容:
*
* 本函数实现获得模块的指定索引的section地址功能。由参数mod的mod->segment指向的是一个由
* grub_dl_segment_t组成的segment链表，该函数就循环扫描该链表的每一项，如果seg->section
* 与参数n相等，那么就返回该项对应的seg->addr；如果没找到，则返回0。
**/
/* Return the address of a section whose index is N.  */
static void *
grub_dl_get_section_addr (grub_dl_t mod, unsigned n)
{
  grub_dl_segment_t seg;

  for (seg = mod->segment; seg; seg = seg->next)
    if (seg->section == n)
      return seg->addr;

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
* @brief 检查参数ehdr是否是有效的ELF文件头部。
*
* @note 注释详细内容:
*
* 本函数实现检查参数ehdr是否是有效的ELF文件头部的功能。首先检查参数size是否小于Elf_Ehdr
* 结构的大小，如果小于则该ehdr无效；接着检查e->e_ident[]对应的魔数以及e->e_version是否合
* 法；之后调用grub_arch_dl_check_header()检查是否符合体系结构对该ehdr的格式要求。
**/

/* Check if EHDR is a valid ELF header.  */
static grub_err_t
grub_dl_check_header (void *ehdr, grub_size_t size)
{
  Elf_Ehdr *e = ehdr;
  grub_err_t err;

  /* Check the header size.  */
  if (size < sizeof (Elf_Ehdr))
    return grub_error (GRUB_ERR_BAD_OS, "ELF header smaller than expected");

  /* Check the magic numbers.  */
  if (e->e_ident[EI_MAG0] != ELFMAG0
      || e->e_ident[EI_MAG1] != ELFMAG1
      || e->e_ident[EI_MAG2] != ELFMAG2
      || e->e_ident[EI_MAG3] != ELFMAG3
      || e->e_ident[EI_VERSION] != EV_CURRENT
      || e->e_version != EV_CURRENT)
    return grub_error (GRUB_ERR_BAD_OS, N_("invalid arch-independent ELF magic"));

  err = grub_arch_dl_check_header (ehdr);
  if (err)
    return err;

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
* @brief 从Elf_Ehdr指定的内存中加载模块的所有段。
*
* @note 注释详细内容:
*
* 本函数实现从Elf_Ehdr指定的内存中加载模块的所有段的功能。首先扫描各个section头部，获得
* section的最大长度和最大对齐值；然后使用得出的最大长度分配模块的section内存；之后再次
* 重新扫描各个section，如果该section的标志位SHF_ALLOC被设置，那么就分配grub_dl_segment_t，
* 如果对应的sh_type是SHT_PROGBITS，则从前面分配的模块的section内存中依次找出sh_addralign
* 对齐的部分内存，从sh_offset偏移处拷贝sh_size大小的section数据到对应的模块section内存；
* 而如果sh_type是SHT_NOBITS，那么直接对该片section内存清零；并将该segment加入mod->segment
* 开头的segment链表中。
**/
/* Load all segments from memory specified by E.  */
static grub_err_t
grub_dl_load_segments (grub_dl_t mod, const Elf_Ehdr *e)
{
  unsigned i;
  Elf_Shdr *s;
  grub_size_t tsize = 0, talign = 1;
#if defined (__ia64__) || defined (__powerpc__)
  grub_size_t tramp;
  grub_size_t got;
#endif
  char *ptr;

  for (i = 0, s = (Elf_Shdr *)((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *)((char *) s + e->e_shentsize))mod->segment
    {
      tsize = ALIGN_UP (tsize, s->sh_addralign) + s->sh_size;/**< section的最大大小 */
      if (talign < s->sh_addralign)
	talign = s->sh_addralign;/**< 各个section的最大对齐值 */
    }

#if defined (__ia64__) || defined (__powerpc__)
  grub_arch_dl_get_tramp_got_size (e, &tramp, &got);
  tramp *= GRUB_ARCH_DL_TRAMP_SIZE;
  got *= sizeof (grub_uint64_t);
  tsize += ALIGN_UP (tramp, GRUB_ARCH_DL_TRAMP_ALIGN);
  if (talign < GRUB_ARCH_DL_TRAMP_ALIGN)
    talign = GRUB_ARCH_DL_TRAMP_ALIGN;
  tsize += ALIGN_UP (got, GRUB_ARCH_DL_GOT_ALIGN);
  if (talign < GRUB_ARCH_DL_GOT_ALIGN)
    talign = GRUB_ARCH_DL_GOT_ALIGN;
#endif

#ifdef GRUB_MACHINE_EMU
  if (talign < 8192 * 16)
    talign = 8192 * 16;
  tsize = ALIGN_UP (tsize, 8192 * 16);
#endif

  mod->base = grub_memalign (talign, tsize);
  if (!mod->base)
    return grub_errno;
  mod->sz = tsize;
  ptr = mod->base;

#ifdef GRUB_MACHINE_EMU
  mprotect (mod->base, tsize, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif

  for (i = 0, s = (Elf_Shdr *)((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *)((char *) s + e->e_shentsize))
    {
      if (s->sh_flags & SHF_ALLOC)
	{
	  grub_dl_segment_t seg;

	  seg = (grub_dl_segment_t) grub_malloc (sizeof (*seg));
	  if (! seg)
	    return grub_errno;

	  if (s->sh_size)
	    {
	      void *addr;

	      ptr = (char *) ALIGN_UP ((grub_addr_t) ptr, s->sh_addralign);
	      addr = ptr;
	      ptr += s->sh_size;

	      switch (s->sh_type)
		{
		case SHT_PROGBITS:
		  grub_memcpy (addr, (char *) e + s->sh_offset, s->sh_size);
		  break;
		case SHT_NOBITS:
		  grub_memset (addr, 0, s->sh_size);
		  break;
		}

	      seg->addr = addr;
	    }
	  else
	    seg->addr = 0;

	  seg->size = s->sh_size;
	  seg->section = i;
	  seg->next = mod->segment;
	  mod->segment = seg;
	}
    }
#if defined (__ia64__) || defined (__powerpc__)
  ptr = (char *) ALIGN_UP ((grub_addr_t) ptr, GRUB_ARCH_DL_TRAMP_ALIGN);
  mod->tramp = ptr;
  ptr += tramp;
  ptr = (char *) ALIGN_UP ((grub_addr_t) ptr, GRUB_ARCH_DL_GOT_ALIGN);
  mod->got = ptr;
  ptr += got;
#endif

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
* @brief 从Elf_Ehdr指定的内存中解析符号。
*
* @note 注释详细内容:
*
* 本函数实现从Elf_Ehdr指定的内存中解析符号的功能。首先扫描各个section头部，找出SHT_SYMTAB
* 类型的section；然后将mod->symtab指向该section所指向的内存；接着解析symtab的各个entry的sym，
* 根据type（STT_NOTYPE，STT_OBJECT，STT_FUNC，STT_SECTION，STT_FILE）分别对其进行处理，最后
* 得出sym->st_value的符号值。特别地，如果是STT_FUNC类型的符号，那么如果name为"grub_mod_init"
* 或者"grub_mod_fini"，则会使用得到的sym->st_value来初始化mod->init或者mod->fini，得到模块的
* 初始化和退出函数指针。
**/
static grub_err_t
grub_dl_resolve_symbols (grub_dl_t mod, Elf_Ehdr *e)
{
  unsigned i;
  Elf_Shdr *s;
  Elf_Sym *sym;
  const char *str;
  Elf_Word size, entsize;

  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_SYMTAB)
      break;

  if (i == e->e_shnum)
    return grub_error (GRUB_ERR_BAD_MODULE, N_("no symbol table"));

#ifdef GRUB_MODULES_MACHINE_READONLY
  mod->symtab = grub_malloc (s->sh_size);
  memcpy (mod->symtab, (char *) e + s->sh_offset, s->sh_size);
#else
  mod->symtab = (Elf_Sym *) ((char *) e + s->sh_offset);
#endif
  sym = mod->symtab;
  size = s->sh_size;
  entsize = s->sh_entsize;

  s = (Elf_Shdr *) ((char *) e + e->e_shoff + e->e_shentsize * s->sh_link);
  str = (char *) e + s->sh_offset;

  for (i = 0;
       i < size / entsize;
       i++, sym = (Elf_Sym *) ((char *) sym + entsize))
    {
      unsigned char type = ELF_ST_TYPE (sym->st_info);
      unsigned char bind = ELF_ST_BIND (sym->st_info);
      const char *name = str + sym->st_name;

      switch (type)
	{
	case STT_NOTYPE:
	case STT_OBJECT:
	  /* Resolve a global symbol.  */
	  if (sym->st_name != 0 && sym->st_shndx == 0)
	    {
	      grub_symbol_t nsym = grub_dl_resolve_symbol (name);
	      if (! nsym)
		return grub_error (GRUB_ERR_BAD_MODULE,
				   N_("symbol `%s' not found"), name);
	      sym->st_value = (Elf_Addr) nsym->addr;
	      if (nsym->isfunc)
		sym->st_info = ELF_ST_INFO (bind, STT_FUNC);
	    }
	  else
	    {
	      sym->st_value += (Elf_Addr) grub_dl_get_section_addr (mod,
								    sym->st_shndx);
	      if (bind != STB_LOCAL)
		if (grub_dl_register_symbol (name, (void *) sym->st_value, 0, mod))
		  return grub_errno;
	    }
	  break;

	case STT_FUNC:
	  sym->st_value += (Elf_Addr) grub_dl_get_section_addr (mod,
								sym->st_shndx);
#ifdef __ia64__
	  {
	      /* FIXME: free descriptor once it's not used anymore. */
	      char **desc;
	      desc = grub_malloc (2 * sizeof (char *));
	      if (!desc)
		return grub_errno;
	      desc[0] = (void *) sym->st_value;
	      desc[1] = mod->base;
	      sym->st_value = (grub_addr_t) desc;
	  }
#endif
	  if (bind != STB_LOCAL)
	    if (grub_dl_register_symbol (name, (void *) sym->st_value, 1, mod))
	      return grub_errno;
	  if (grub_strcmp (name, "grub_mod_init") == 0)
	    mod->init = (void (*) (grub_dl_t)) sym->st_value;
	  else if (grub_strcmp (name, "grub_mod_fini") == 0)
	    mod->fini = (void (*) (void)) sym->st_value;
	  break;

	case STT_SECTION:
	  sym->st_value = (Elf_Addr) grub_dl_get_section_addr (mod,
							       sym->st_shndx);
	  break;

	case STT_FILE:
	  sym->st_value = 0;
	  break;

	default:
	  return grub_error (GRUB_ERR_BAD_MODULE,
			     "unknown symbol type `%d'", (int) type);
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
* @date 注释添加日期：2013年6月8日
*
* @brief 调用模块的初始化函数。
*
* @note 注释详细内容:
*
* 本函数实现调用模块的初始化函数（即mod->init）的功能。
**/
static void
grub_dl_call_init (grub_dl_t mod)
{
  if (mod->init)
    (mod->init) (mod);
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
* @brief 检查模块的版权信息。
*
* @note 注释详细内容:
*
* 本函数实现检查模块的版权信息的功能。比较所有的section，如果sh_name为".module_license"，
* 则对比该section的内容是否与"LICENSE=GPLv3"，"LICENSE=GPLv3+"或者"LICENSE=GPLv2+"一致，
* 一致的话返回GRUB_ERR_NONE，否则就返回GRUB_ERR_BAD_MODULE。
**/
/* Me, Vladimir Serbinenko, hereby I add this module check as per new
   GNU module policy. Note that this license check is informative only.
   Modules have to be licensed under GPLv3 or GPLv3+ (optionally
   multi-licensed under other licences as well) independently of the
   presence of this check and solely by linking (module loading in GRUB
   constitutes linking) and GRUB core being licensed under GPLv3+.
   Be sure to understand your license obligations.
*/
static grub_err_t
grub_dl_check_license (Elf_Ehdr *e)
{
  Elf_Shdr *s;
  const char *str;
  unsigned i;

  s = (Elf_Shdr *) ((char *) e + e->e_shoff + e->e_shstrndx * e->e_shentsize);
  str = (char *) e + s->sh_offset;

  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (grub_strcmp (str + s->sh_name, ".module_license") == 0)
      {
	if (grub_strcmp ((char *) e + s->sh_offset, "LICENSE=GPLv3") == 0
	    || grub_strcmp ((char *) e + s->sh_offset, "LICENSE=GPLv3+") == 0
	    || grub_strcmp ((char *) e + s->sh_offset, "LICENSE=GPLv2+") == 0)
	  return GRUB_ERR_NONE;
      }

  return grub_error (GRUB_ERR_BAD_MODULE, "incompatible license");
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
* @brief 解析模块的名字。
*
* @note 注释详细内容:
*
* 本函数实现解析模块的名字的功能。扫描各个section，如果section的sh_name为".modname"，那么
* 就将其sh_offset处对应的内容保存为mod->name，从而得到模块的名字。
**/
static grub_err_t
grub_dl_resolve_name (grub_dl_t mod, Elf_Ehdr *e)
{
  Elf_Shdr *s;
  const char *str;
  unsigned i;

  s = (Elf_Shdr *) ((char *) e + e->e_shoff + e->e_shstrndx * e->e_shentsize);
  str = (char *) e + s->sh_offset;

  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (grub_strcmp (str + s->sh_name, ".modname") == 0)
      {
	mod->name = grub_strdup ((char *) e + s->sh_offset);
	if (! mod->name)
	  return grub_errno;
	break;
      }

  if (i == e->e_shnum)
    return grub_error (GRUB_ERR_BAD_MODULE, "no module name found");

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
* @brief 解析模块的依赖，构建该模块依赖的模块链表。
*
* @note 注释详细内容:
*
* 本函数实现解析模块的名字的功能。扫描各个section，如果section的sh_name为".moddeps"，那么
* 就从其sh_offset处对应的内容开始，将其当做名字字符串name，从而得到依赖模块的名字；用此
* 名字调用grub_dl_load()函数加载该依赖模块，并调用grub_dl_ref()对齐进行引用；并分配一个新
* 的grub_dl_dep_t，将这个依赖模块挂接到mod->dep开始的依赖模块链表中。
**/
static grub_err_t
grub_dl_resolve_dependencies (grub_dl_t mod, Elf_Ehdr *e)
{
  Elf_Shdr *s;
  const char *str;
  unsigned i;

  s = (Elf_Shdr *) ((char *) e + e->e_shoff + e->e_shstrndx * e->e_shentsize);
  str = (char *) e + s->sh_offset;

  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (grub_strcmp (str + s->sh_name, ".moddeps") == 0)
      {
	const char *name = (char *) e + s->sh_offset;
	const char *max = name + s->sh_size;

	while ((name < max) && (*name))
	  {
	    grub_dl_t m;
	    grub_dl_dep_t dep;

	    m = grub_dl_load (name);
	    if (! m)
	      return grub_errno;

	    grub_dl_ref (m);

	    dep = (grub_dl_dep_t) grub_malloc (sizeof (*dep));
	    if (! dep)
	      return grub_errno;

	    dep->mod = m;
	    dep->next = mod->dep;
	    mod->dep = dep;

	    name += grub_strlen (name) + 1;
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
* @date 注释添加日期：2013年6月8日
*
* @brief 对模块进行引用（增加引用计数）。
*
* @note 注释详细内容:
*
* 本函数实现对模块进行引用（增加引用计数）的功能。对于mod->dep开始的依赖模块链表中的每一个
* 依赖模块，都调用grub_dl_ref()进行递归引用；之后对mod->ref_count递增，从而实现递归引用模块。
**/
int
grub_dl_ref (grub_dl_t mod)
{
  grub_dl_dep_t dep;

  if (!mod)
    return 0;

  for (dep = mod->dep; dep; dep = dep->next)
    grub_dl_ref (dep->mod);

  return ++mod->ref_count;
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
* @brief 对模块进行解除引用（递减引用计数）。
*
* @note 注释详细内容:
*
* 本函数实现对模块进行解除引用（递减引用计数）的功能。对于mod->dep开始的依赖模块链表中每个
* 依赖模块，都调用grub_dl_unref()进行递归解除引用；之后对mod->ref_count递减，从而实现递归解
* 除模块的引用。
**/
int
grub_dl_unref (grub_dl_t mod)
{
  grub_dl_dep_t dep;

  if (!mod)
    return 0;

  for (dep = mod->dep; dep; dep = dep->next)
    grub_dl_unref (dep->mod);

  return --mod->ref_count;
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
* @brief 同步模块的cache。
*
* @note 注释详细内容:
*
* 本函数实现同步模块的cache的功能。对于模块的section内存调用grub_arch_sync_caches()来冲刷
* cache到内存，实现模块的cache同步。
**/
static void
grub_dl_flush_cache (grub_dl_t mod)
{
  grub_dprintf ("modules", "flushing 0x%lx bytes at %p\n",
		(unsigned long) mod->sz, mod->base);
  grub_arch_sync_caches (mod->base, mod->sz);
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
* @brief 模块加载的核心代码。
*
* @note 注释详细内容:
*
* 本函数实现模块加载的核心代码的功能。从指定的地址处加载模块，首先使用grub_dl_check_header()
* 验证该内存地址是一个合法的模块；并且要求该模块是可重定位的（ET_REL）；并且确保每个section
* 都模块的运行内存范围内；之后分配grub_dl_t mod，并陆续调用子函数检查模块的版权，解析名字，
* 解析模块的依赖关系，加载各个段，解析符号，并且重新定位符号；如果前面的操作都成功，则模块
* 已经加载到内存并经过了验证，于是grub_dl_flush_cache()来同步模块内存；之后grub_dl_call_init()
* 来初始化模块；最后将该模块通过grub_dl_add()加入全局模块链表。
**/
/* Load a module from core memory.  */
grub_dl_t
grub_dl_load_core (void *addr, grub_size_t size)
{
  Elf_Ehdr *e;
  grub_dl_t mod;

  grub_dprintf ("modules", "module at %p, size 0x%lx\n", addr,
		(unsigned long) size);
  e = addr;
  if (grub_dl_check_header (e, size))
    return 0;

  if (e->e_type != ET_REL)
    {
      grub_error (GRUB_ERR_BAD_MODULE, N_("this ELF file is not of the right type"));
      return 0;
    }

  /* Make sure that every section is within the core.  */
  if (size < e->e_shoff + e->e_shentsize * e->e_shnum)
    {
      grub_error (GRUB_ERR_BAD_OS, "ELF sections outside core");
      return 0;
    }

  mod = (grub_dl_t) grub_zalloc (sizeof (*mod));
  if (! mod)
    return 0;

  mod->ref_count = 1;

  grub_dprintf ("modules", "relocating to %p\n", mod);
  /* Me, Vladimir Serbinenko, hereby I add this module check as per new
     GNU module policy. Note that this license check is informative only.
     Modules have to be licensed under GPLv3 or GPLv3+ (optionally
     multi-licensed under other licences as well) independently of the
     presence of this check and solely by linking (module loading in GRUB
     constitutes linking) and GRUB core being licensed under GPLv3+.
     Be sure to understand your license obligations.
  */
  if (grub_dl_check_license (e)
      || grub_dl_resolve_name (mod, e)
      || grub_dl_resolve_dependencies (mod, e)
      || grub_dl_load_segments (mod, e)
      || grub_dl_resolve_symbols (mod, e)
      || grub_arch_dl_relocate_symbols (mod, e))
    {
      mod->fini = 0;
      grub_dl_unload (mod);
      return 0;
    }

  grub_dl_flush_cache (mod);

  grub_dprintf ("modules", "module name: %s\n", mod->name);
  grub_dprintf ("modules", "init function: %p\n", mod->init);
  grub_dl_call_init (mod);

  if (grub_dl_add (mod))
    {
      grub_dl_unload (mod);
      return 0;
    }

  return mod;
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
* @brief 从文件加载模块进入内存并初始化。
*
* @note 注释详细内容:
*
* 本函数实现从文件加载模块进入内存并初始化的功能。首先调用grub_file_open()打开文件，分配
* 文件内存，并且调用grub_file_read()读入模块的数据到内存，并关闭该模块文件；最后调用函数
* grub_dl_load_core()来实际将模块加载到恰当位置，并做适当清理。
**/
/* Load a module from the file FILENAME.  */
grub_dl_t
grub_dl_load_file (const char *filename)
{
  grub_file_t file = NULL;
  grub_ssize_t size;
  void *core = 0;
  grub_dl_t mod = 0;

  file = grub_file_open (filename);
  if (! file)
    return 0;

  size = grub_file_size (file);
  core = grub_malloc (size);
  if (! core)
    {
      grub_file_close (file);
      return 0;
    }

  if (grub_file_read (file, core, size) != (int) size)
    {
      grub_file_close (file);
      grub_free (core);
      return 0;
    }

  /* We must close this before we try to process dependencies.
     Some disk backends do not handle gracefully multiple concurrent
     opens of the same device.  */
  grub_file_close (file);

  mod = grub_dl_load_core (core, size);
  grub_free (core);
  if (! mod)
    return 0;

  mod->ref_count--;
  return mod;
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
* @brief 按照文件名从文件加载模块进入内存并初始化。
*
* @note 注释详细内容:
*
* 本函数实现按照文件名从文件加载模块进入内存并初始化的功能。首先调用grub_dl_get()看在
* 已经加载的模块链表中是否已经有该模块了，如果有则直接返回该模块；接着按照目标CPU和平
* 台来找到模块(*.mod)的文件名，接着使用这个文件名来使用grub_dl_load_file()实际加载该模
* 块文件。
**/
/* Load a module using a symbolic name.  */
grub_dl_t
grub_dl_load (const char *name)
{
  char *filename;
  grub_dl_t mod;
  const char *grub_dl_dir = grub_env_get ("prefix");

  mod = grub_dl_get (name);
  if (mod)
    return mod;

  if (! grub_dl_dir) {
    grub_error (GRUB_ERR_FILE_NOT_FOUND, N_("variable `%s' isn't set"), "prefix");
    return 0;
  }

  filename = grub_xasprintf ("%s/" GRUB_TARGET_CPU "-" GRUB_PLATFORM "/%s.mod",
			     grub_dl_dir, name);
  if (! filename)
    return 0;

  mod = grub_dl_load_file (filename);
  grub_free (filename);

  if (! mod)
    return 0;

  if (grub_strcmp (mod->name, name) != 0)
    grub_error (GRUB_ERR_BAD_MODULE, "mismatched names");

  return mod;
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
* @brief 卸载模块。
*
* @note 注释详细内容:
*
* 本函数实现卸载模块的功能。如果引用计数大于0，则不作操作；之后如果有mod->fini函数则调用
* 该函数；接着使用grub_dl_remove()将模块从全局模块链表中删除；用grub_dl_unregister_symbols
* 来取消注册的符号；接着对mod->dep开始的每个依赖模块都调用grub_dl_unload()来卸载依赖的模
* 块；最后释放模块资源。
**/
/* Unload the module MOD.  */
int
grub_dl_unload (grub_dl_t mod)
{
  grub_dl_dep_t dep, depn;

  if (mod->ref_count > 0)
    return 0;

  if (mod->fini)
    (mod->fini) ();

  grub_dl_remove (mod);
  grub_dl_unregister_symbols (mod);

  for (dep = mod->dep; dep; dep = depn)
    {
      depn = dep->next;

      grub_dl_unload (dep->mod);

      grub_free (dep);
    }

  grub_free (mod->base);
  grub_free (mod->name);
#ifdef GRUB_MODULES_MACHINE_READONLY
  grub_free (mod->symtab);
#endif
  grub_free (mod);
  return 1;
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
* @brief 卸载所有不需要的模块。
*
* @note 注释详细内容:
*
* 本函数实现卸载所有不需要的模块的功能。对grub_dl_head开头的每个模块，都调用函数
* grub_dl_unload()来卸载。
**/
/* Unload unneeded modules.  */
void
grub_dl_unload_unneeded (void)
{
  /* Because grub_dl_remove modifies the list of modules, this
     implementation is tricky.  */
  grub_dl_t p = grub_dl_head;

  while (p)
    {
      if (grub_dl_unload (p))
	{
	  p = grub_dl_head;
	  continue;
	}

      p = p->next;
    }
}
