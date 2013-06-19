/* elf.c - load ELF files */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/err.h>
#include <grub/elf.h>
#include <grub/elfload.h>
#include <grub/file.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/dl.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

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
* @brief 检查是否为合法的ELF文件头。
*
* @note 注释详细内容:
*
* 本函数实现检查是否为合法的ELF文件头的功能。检查e->e_ident[]对应的魔数以及e->e_version是
* 否合法。
**/
/* Check if EHDR is a valid ELF header.  */
static grub_err_t
grub_elf_check_header (grub_elf_t elf)
{
  Elf32_Ehdr *e = &elf->ehdr.ehdr32;

  if (e->e_ident[EI_MAG0] != ELFMAG0
      || e->e_ident[EI_MAG1] != ELFMAG1
      || e->e_ident[EI_MAG2] != ELFMAG2
      || e->e_ident[EI_MAG3] != ELFMAG3
      || e->e_ident[EI_VERSION] != EV_CURRENT
      || e->e_version != EV_CURRENT)
    return grub_error (GRUB_ERR_BAD_OS, N_("invalid arch-independent ELF magic"));

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
* @brief 关闭一个ELF文件。
*
* @note 注释详细内容:
*
* 本函数实现关闭一个ELF文件的功能。释放对应的elf->phdrs和elf本身，并且调用grub_file_close()
* 关闭文件本身。
**/
grub_err_t
grub_elf_close (grub_elf_t elf)
{
  grub_file_t file = elf->file;

  grub_free (elf->phdrs);
  grub_free (elf);

  if (file)
    grub_file_close (file);

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
* @brief 检查一个已经打开的文件是否是ELF文件。
*
* @note 注释详细内容:
*
* 本函数实现检查一个已经打开的文件是否是ELF文件的功能。首先分配一个grub_elf_t格式的数据
* 结构；接着使用grub_file_seek()将文件偏移量定位到0；然后使用grub_file_read()读入ehdr大小
* 的数据（ELF头部的大小），然后使用grub_elf_check_header()校验该头部数据是否符合ELF规范。
* 如果是ELF文件，则返回刚才分配的grub_elf_t结构体。
**/
grub_elf_t
grub_elf_file (grub_file_t file, const char *filename)
{
  grub_elf_t elf;

  elf = grub_zalloc (sizeof (*elf));
  if (! elf)
    return 0;

  elf->file = file;

  if (grub_file_seek (elf->file, 0) == (grub_off_t) -1)
    goto fail;

  if (grub_file_read (elf->file, &elf->ehdr, sizeof (elf->ehdr))
      != sizeof (elf->ehdr))
    {
      if (!grub_errno)
	grub_error (GRUB_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
		    filename);
      goto fail;
    }

  if (grub_elf_check_header (elf))
    goto fail;

  return elf;

fail:
  grub_free (elf->phdrs);
  grub_free (elf);
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
* @brief 打开文件并验证是否是ELF文件（并返回grub_elf_t结构）。
*
* @note 注释详细内容:
*
* 本函数实现打开文件并验证是否是ELF文件（并返回grub_elf_t结构）的功能。先用grub_file_open()
* 打开文件，并使用grub_elf_file()验证该文件是否是ELF文件；如不是ELF文件则用grub_file_close()
* 来关闭文件，否则返回grub_elf_t结构。
**/
grub_elf_t
grub_elf_open (const char *name)
{
  grub_file_t file;
  grub_elf_t elf;

  file = grub_file_open (name);
  if (! file)
    return 0;

  elf = grub_elf_file (file, name);
  if (! elf)
    grub_file_close (file);

  return elf;
}


/* 32-bit */

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
* @brief 判断是否是32位ELF文件。
*
* @note 注释详细内容:
*
* 本函数实现判断是否是32位ELF文件的功能。比较ehdr.ehdr32.e_ident[EI_CLASS] == ELFCLASS32。
**/
int
grub_elf_is_elf32 (grub_elf_t elf)
{
  return elf->ehdr.ehdr32.e_ident[EI_CLASS] == ELFCLASS32;
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
* @brief 加载32位ELF文件的程序头。
*
* @note 注释详细内容:
*
* 本函数实现加载32位ELF文件的程序头的功能。首先获得该文件的程序头的整体大小；然后分配
* 对应的内存；并用grub_file_seek()将文件定位到ehdr.ehdr32.e_phoff处；再用grub_file_read()
* 读取对应大小的phdrs。
**/
static grub_err_t
grub_elf32_load_phdrs (grub_elf_t elf, const char *filename)
{
  grub_ssize_t phdrs_size;

  phdrs_size = elf->ehdr.ehdr32.e_phnum * elf->ehdr.ehdr32.e_phentsize;

  grub_dprintf ("elf", "Loading program headers at 0x%llx, size 0x%lx.\n",
		(unsigned long long) elf->ehdr.ehdr32.e_phoff,
		(unsigned long) phdrs_size);

  elf->phdrs = grub_malloc (phdrs_size);
  if (! elf->phdrs)
    return grub_errno;

  if ((grub_file_seek (elf->file, elf->ehdr.ehdr32.e_phoff) == (grub_off_t) -1)
      || (grub_file_read (elf->file, elf->phdrs, phdrs_size) != phdrs_size))
    {
      if (!grub_errno)
	grub_error (GRUB_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
		    filename);
      return grub_errno;
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
* @brief 枚举32位ELF文件的程序头中的每个程序头项，并调用回调函数。
*
* @note 注释详细内容:
*
* 本函数实现枚举32位ELF文件的程序头中的每个程序头项，并调用回调函数的功能。首先使用函数
* grub_elf32_load_phdrs()加载程序头；然后对ehdr.ehdr32.e_phnum个程序头，每个都调用参数给
* 的hook函数指针。
**/
grub_err_t
grub_elf32_phdr_iterate (grub_elf_t elf,
			 const char *filename,
			 int NESTED_FUNC_ATTR (*hook) (grub_elf_t, Elf32_Phdr *, void *),
			 void *hook_arg)
{
  Elf32_Phdr *phdrs;
  unsigned int i;

  if (! elf->phdrs)
    if (grub_elf32_load_phdrs (elf, filename))
      return grub_errno;
  phdrs = elf->phdrs;

  for (i = 0; i < elf->ehdr.ehdr32.e_phnum; i++)
    {
      Elf32_Phdr *phdr = phdrs + i;
      grub_dprintf ("elf",
		    "Segment %u: type 0x%x paddr 0x%lx memsz 0x%lx "
		    "filesz %lx\n",
		    i, phdr->p_type,
		    (unsigned long) phdr->p_paddr,
		    (unsigned long) phdr->p_memsz,
		    (unsigned long) phdr->p_filesz);
      if (hook (elf, phdr, hook_arg))
	break;
    }

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
* @brief 计算32位ELF文件的所有段占用的大小。
*
* @note 注释详细内容:
*
* 本函数实现计算32位ELF文件的所有段占用的大小的功能。使用grub_elf32_phdr_iterate()枚举每个
* 程序头，回调内嵌函数calcsize()来统计所有段占据的总体内存大小。
**/
/* Calculate the amount of memory spanned by the segments.  */
grub_size_t
grub_elf32_size (grub_elf_t elf, const char *filename,
		 Elf32_Addr *base, grub_uint32_t *max_align)
{
  Elf32_Addr segments_start = (Elf32_Addr) -1;
  Elf32_Addr segments_end = 0;
  int nr_phdrs = 0;
  grub_uint32_t curr_align = 1;

  /* Run through the program headers to calculate the total memory size we
   * should claim.  */
  auto int NESTED_FUNC_ATTR calcsize (grub_elf_t _elf, Elf32_Phdr *phdr, void *_arg);
  int NESTED_FUNC_ATTR calcsize (grub_elf_t _elf  __attribute__ ((unused)),
				 Elf32_Phdr *phdr,
				 void *_arg __attribute__ ((unused)))
    {
      /* Only consider loadable segments.  */
      if (phdr->p_type != PT_LOAD)
	return 0;
      nr_phdrs++;
      if (phdr->p_paddr < segments_start)
	segments_start = phdr->p_paddr;
      if (phdr->p_paddr + phdr->p_memsz > segments_end)
	segments_end = phdr->p_paddr + phdr->p_memsz;
      if (curr_align < phdr->p_align)
	curr_align = phdr->p_align;
      return 0;
    }

  grub_elf32_phdr_iterate (elf, filename, calcsize, 0);

  if (base)
    *base = 0;

  if (nr_phdrs == 0)
    {
      grub_error (GRUB_ERR_BAD_OS, "no program headers present");
      return 0;
    }

  if (segments_end < segments_start)
    {
      /* Very bad addresses.  */
      grub_error (GRUB_ERR_BAD_OS, "bad program header load addresses");
      return 0;
    }

  if (base)
    *base = segments_start;
  if (max_align)
    *max_align = curr_align;
  return segments_end - segments_start;
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
* @brief 加载32位ELF文件的所有可加载段进入内存。
*
* @note 注释详细内容:
*
* 本函数实现加载32位ELF文件的所有可加载段进入内存的功能。使用grub_elf32_phdr_iterate()枚举每个
* 程序头，回调内嵌函数grub_elf32_load_segment()来读取所有段占据的内存。
**/
/* Load every loadable segment into memory specified by `_load_hook'.  */
grub_err_t
grub_elf32_load (grub_elf_t _elf, const char *filename,
		 grub_elf32_load_hook_t _load_hook,
		 grub_addr_t *base, grub_size_t *size)
{
  grub_addr_t load_base = (grub_addr_t) -1ULL;
  grub_size_t load_size = 0;
  grub_err_t err;

  auto int NESTED_FUNC_ATTR grub_elf32_load_segment (grub_elf_t elf, Elf32_Phdr *phdr, void *hook);
  int NESTED_FUNC_ATTR grub_elf32_load_segment (grub_elf_t elf, Elf32_Phdr *phdr, void *hook)
  {
    grub_elf32_load_hook_t load_hook = (grub_elf32_load_hook_t) hook;
    grub_addr_t load_addr;
    int do_load = 1;

    load_addr = phdr->p_paddr;
    if (load_hook && load_hook (phdr, &load_addr, &do_load))
      return 1;

    if (! do_load)
      return 0;

    if (load_addr < load_base)
      load_base = load_addr;

    grub_dprintf ("elf", "Loading segment at 0x%llx, size 0x%llx\n",
		  (unsigned long long) load_addr,
		  (unsigned long long) phdr->p_memsz);

    if (grub_file_seek (elf->file, phdr->p_offset) == (grub_off_t) -1)
      return grub_errno;

    if (phdr->p_filesz)
      {
	grub_ssize_t read;
	read = grub_file_read (elf->file, (void *) load_addr, phdr->p_filesz);
	if (read != (grub_ssize_t) phdr->p_filesz)
	  {
	    /* XXX How can we free memory from `load_hook'? */
	    if (!grub_errno)
	      grub_error (GRUB_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
			  filename);
	    return grub_errno;
	  }
      }

    if (phdr->p_filesz < phdr->p_memsz)
      grub_memset ((void *) (long) (load_addr + phdr->p_filesz),
		   0, phdr->p_memsz - phdr->p_filesz);

    load_size += phdr->p_memsz;

    return 0;
  }

  err = grub_elf32_phdr_iterate (_elf, filename,
				 grub_elf32_load_segment, _load_hook);

  if (base)
    *base = load_base;
  if (size)
    *size = load_size;

  return err;
}


/* 64-bit */
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
* @brief 判断是否是64位ELF文件。
*
* @note 注释详细内容:
*
* 本函数实现判断是否是64位ELF文件的功能。比较ehdr.ehdr64.e_ident[EI_CLASS] == ELFCLASS64。
**/
int
grub_elf_is_elf64 (grub_elf_t elf)
{
  return elf->ehdr.ehdr64.e_ident[EI_CLASS] == ELFCLASS64;
}

static grub_err_t
grub_elf64_load_phdrs (grub_elf_t elf, const char *filename)
{
  grub_ssize_t phdrs_size;

  phdrs_size = elf->ehdr.ehdr64.e_phnum * elf->ehdr.ehdr64.e_phentsize;

  grub_dprintf ("elf", "Loading program headers at 0x%llx, size 0x%lx.\n",
		(unsigned long long) elf->ehdr.ehdr64.e_phoff,
		(unsigned long) phdrs_size);

  elf->phdrs = grub_malloc (phdrs_size);
  if (! elf->phdrs)
    return grub_errno;

  if ((grub_file_seek (elf->file, elf->ehdr.ehdr64.e_phoff) == (grub_off_t) -1)
      || (grub_file_read (elf->file, elf->phdrs, phdrs_size) != phdrs_size))
    {
      if (!grub_errno)
	grub_error (GRUB_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
		    filename);
      return grub_errno;
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
* @brief 枚举64位ELF文件的程序头中的每个程序头项，并调用回调函数。
*
* @note 注释详细内容:
*
* 本函数实现枚举32位ELF文件的程序头中的每个程序头项，并调用回调函数的功能。首先使用函数
* grub_elf64_load_phdrs()加载程序头；然后对ehdr.ehdr64.e_phnum个程序头，每个都调用参数给
* 的hook函数指针。
**/
grub_err_t
grub_elf64_phdr_iterate (grub_elf_t elf,
			 const char *filename,
			 int NESTED_FUNC_ATTR (*hook) (grub_elf_t, Elf64_Phdr *, void *),
			 void *hook_arg)
{
  Elf64_Phdr *phdrs;
  unsigned int i;

  if (! elf->phdrs)
    if (grub_elf64_load_phdrs (elf, filename))
      return grub_errno;
  phdrs = elf->phdrs;

  for (i = 0; i < elf->ehdr.ehdr64.e_phnum; i++)
    {
      Elf64_Phdr *phdr = phdrs + i;
      grub_dprintf ("elf",
		    "Segment %u: type 0x%x paddr 0x%lx memsz 0x%lx "
		    "filesz %lx\n",
		    i, phdr->p_type,
		    (unsigned long) phdr->p_paddr,
		    (unsigned long) phdr->p_memsz,
		    (unsigned long) phdr->p_filesz);
      if (hook (elf, phdr, hook_arg))
	break;
    }

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
* @brief 计算32位ELF文件的所有段占用的大小。
*
* @note 注释详细内容:
*
* 本函数实现计算32位ELF文件的所有段占用的大小的功能。使用grub_elf32_phdr_iterate()枚举每个
* 程序头，回调内嵌函数calcsize()来统计所有段占据的总体内存大小。
**/
/* Calculate the amount of memory spanned by the segments.  */
grub_size_t
grub_elf64_size (grub_elf_t elf, const char *filename,
		 Elf64_Addr *base, grub_uint64_t *max_align)
{
  Elf64_Addr segments_start = (Elf64_Addr) -1;
  Elf64_Addr segments_end = 0;
  int nr_phdrs = 0;
  grub_uint64_t curr_align = 1;

  /* Run through the program headers to calculate the total memory size we
   * should claim.  */
  auto int NESTED_FUNC_ATTR calcsize (grub_elf_t _elf, Elf64_Phdr *phdr, void *_arg);
  int NESTED_FUNC_ATTR calcsize (grub_elf_t _elf  __attribute__ ((unused)),
				 Elf64_Phdr *phdr,
				 void *_arg __attribute__ ((unused)))
    {
      /* Only consider loadable segments.  */
      if (phdr->p_type != PT_LOAD)
	return 0;
      nr_phdrs++;
      if (phdr->p_paddr < segments_start)
	segments_start = phdr->p_paddr;
      if (phdr->p_paddr + phdr->p_memsz > segments_end)
	segments_end = phdr->p_paddr + phdr->p_memsz;
      if (curr_align < phdr->p_align)
	curr_align = phdr->p_align;
      return 0;
    }

  grub_elf64_phdr_iterate (elf, filename, calcsize, 0);

  if (base)
    *base = 0;

  if (nr_phdrs == 0)
    {
      grub_error (GRUB_ERR_BAD_OS, "no program headers present");
      return 0;
    }

  if (segments_end < segments_start)
    {
      /* Very bad addresses.  */
      grub_error (GRUB_ERR_BAD_OS, "bad program header load addresses");
      return 0;
    }

  if (base)
    *base = segments_start;
  if (max_align)
    *max_align = curr_align;
  return segments_end - segments_start;
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
* @brief 加载64位ELF文件的所有可加载段进入内存。
*
* @note 注释详细内容:
*
* 本函数实现加载32位ELF文件的所有可加载段进入内存的功能。使用grub_elf64_phdr_iterate()枚举每个
* 程序头，回调内嵌函数grub_elf64_load_segment()来读取所有段占据的内存。
**/
/* Load every loadable segment into memory specified by `_load_hook'.  */
grub_err_t
grub_elf64_load (grub_elf_t _elf, const char *filename,
		 grub_elf64_load_hook_t _load_hook,
		 grub_addr_t *base, grub_size_t *size)
{
  grub_addr_t load_base = (grub_addr_t) -1ULL;
  grub_size_t load_size = 0;
  grub_err_t err;

  auto int NESTED_FUNC_ATTR grub_elf64_load_segment (grub_elf_t elf, Elf64_Phdr *phdr,
						     void *hook);
  int NESTED_FUNC_ATTR grub_elf64_load_segment (grub_elf_t elf, Elf64_Phdr *phdr, void *hook)
  {
    grub_elf64_load_hook_t load_hook = (grub_elf64_load_hook_t) hook;
    grub_addr_t load_addr;
    int do_load = 1;

    load_addr = phdr->p_paddr;
    if (load_hook && load_hook (phdr, &load_addr, &do_load))
      return 1;

    if (! do_load)
      return 0;

    if (load_addr < load_base)
      load_base = load_addr;

    grub_dprintf ("elf", "Loading segment at 0x%llx, size 0x%llx\n",
		  (unsigned long long) load_addr,
		  (unsigned long long) phdr->p_memsz);

    if (grub_file_seek (elf->file, phdr->p_offset) == (grub_off_t) -1)
      return grub_errno;

    if (phdr->p_filesz)
      {
	grub_ssize_t read;
	read = grub_file_read (elf->file, (void *) load_addr, phdr->p_filesz);
	if (read != (grub_ssize_t) phdr->p_filesz)
          {
	    /* XXX How can we free memory from `load_hook'?  */
	    if (!grub_errno)
	      grub_error (GRUB_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
			  filename);
	    return grub_errno;
          }
      }

    if (phdr->p_filesz < phdr->p_memsz)
      grub_memset ((void *) (long) (load_addr + phdr->p_filesz),
		   0, phdr->p_memsz - phdr->p_filesz);

    load_size += phdr->p_memsz;

    return 0;
  }

  err = grub_elf64_phdr_iterate (_elf, filename,
				 grub_elf64_load_segment, _load_hook);

  if (base)
    *base = load_base;
  if (size)
    *size = load_size;

  return err;
}
