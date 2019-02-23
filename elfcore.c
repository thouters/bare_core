/* Copyright (c) 2005-2007, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ---
 * Author: Markus Gutschke
 */

#include "elfcore.h"

#include <libelf/libelf.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <unistd.h>

#include <assert.h>

#ifndef CLONE_UNTRACED
#define CLONE_UNTRACED 0x00800000
#endif

/* Data structures found in x86-32/64 and ARM core dumps on Linux; similar
 * data structures are defined in /usr/include/{linux,asm}/... but those
 * headers conflict with the rest of the libc headers. So we cannot
 * include them here.
 */

  typedef struct fpxregs {      /* No extended FPU registers on ARM          */
  } fpxregs;
  typedef struct fpregs {       /* FPU registers                             */
    struct fp_reg {
      unsigned int sign1:1;
      unsigned int unused:15;
      unsigned int sign2:1;
      unsigned int exponent:14;
      unsigned int j:1;
      unsigned int mantissa1:31;
      unsigned int mantissa0:32;
    } fpregs[8];
    unsigned int   fpsr:32;
    unsigned int   fpcr:32;
    unsigned char  ftype[8];
    unsigned int   init_flag;
  } fpregs;
  #define regs arm_regs         /* General purpose registers                 */

typedef struct elf_timeval {    /* Time value with microsecond resolution    */
  long tv_sec;                  /* Seconds                                   */
  long tv_usec;                 /* Microseconds                              */
} elf_timeval;


typedef struct elf_siginfo {    /* Information about signal (unused)         */
  int32_t si_signo;             /* Signal number                             */
  int32_t si_code;              /* Extra code                                */
  int32_t si_errno;             /* Errno                                     */
} elf_siginfo;


typedef struct prstatus {       /* Information about thread; includes CPU reg*/
  elf_siginfo    pr_info;       /* Info associated with signal               */
  uint16_t       pr_cursig;     /* Current signal                            */
  unsigned long  pr_sigpend;    /* Set of pending signals                    */
  unsigned long  pr_sighold;    /* Set of held signals                       */
  pid_t          pr_pid;        /* Process ID                                */
  pid_t          pr_ppid;       /* Parent's process ID                       */
  pid_t          pr_pgrp;       /* Group ID                                  */
  pid_t          pr_sid;        /* Session ID                                */
  elf_timeval    pr_utime;      /* User time                                 */
  elf_timeval    pr_stime;      /* System time                               */
  elf_timeval    pr_cutime;     /* Cumulative user time                      */
  elf_timeval    pr_cstime;     /* Cumulative system time                    */
  regs           pr_reg;        /* CPU registers                             */
  uint32_t       pr_fpvalid;    /* True if math co-processor being used      */
} prstatus;


typedef struct prpsinfo {       /* Information about process                 */
  unsigned char  pr_state;      /* Numeric process state                     */
  char           pr_sname;      /* Char for pr_state                         */
  unsigned char  pr_zomb;       /* Zombie                                    */
  signed char    pr_nice;       /* Nice val                                  */
  unsigned long  pr_flag;       /* Flags                                     */
  uint16_t       pr_uid;        /* User ID                                   */
  uint16_t       pr_gid;        /* Group ID                                  */
  pid_t          pr_pid;        /* Process ID                                */
  pid_t          pr_ppid;       /* Parent's process ID                       */
  pid_t          pr_pgrp;       /* Group ID                                  */
  pid_t          pr_sid;        /* Session ID                                */
  char           pr_fname[16];  /* Filename of executable                    */
  char           pr_psargs[80]; /* Initial part of arg list                  */
} prpsinfo;


typedef struct user {           /* Ptrace returns this data for thread state */
  regs           regs;          /* CPU registers                             */
  unsigned long  fpvalid;       /* True if math co-processor being used      */
  unsigned long  tsize;         /* Text segment size in pages                */
  unsigned long  dsize;         /* Data segment size in pages                */
  unsigned long  ssize;         /* Stack segment size in pages               */
  unsigned long  start_code;    /* Starting virtual address of text          */
  unsigned long  start_stack;   /* Starting virtual address of stack area    */
  unsigned long  signal;        /* Signal that caused the core dump          */
  unsigned long  reserved;      /* No longer used                            */
  regs           *regs_ptr;     /* Used by gdb to help find the CPU registers*/
  unsigned long  magic;         /* Magic for old A.OUT core files            */
  char           comm[32];      /* User command that was responsible         */
  unsigned long  debugreg[8];
  fpregs         fpregs;        /* FPU registers                             */
  fpregs         *fpregs_ptr;   /* Pointer to FPU registers                  */
} user;



  #define ELF_CLASS ELFCLASS32
  #define Ehdr      Elf32_Ehdr
  #define Phdr      Elf32_Phdr
  #define Shdr      Elf32_Shdr
  #define Nhdr      Elf32_Nhdr


  #define ELF_ARCH  EM_ARM


/* Re-runs fn until it doesn't cause EINTR
 */
#define NO_INTR(fn)    do {} while ((fn) < 0 && errno == EINTR)


/* Wrapper for write() which is guaranteed to never return EINTR nor
 * short writes.
 */
ssize_t c_write(int f, const void *void_buf, size_t bytes)
{
    const unsigned char *buf = (const unsigned char*)void_buf;
    size_t len = bytes;
    while (len > 0) {
      ssize_t rc;
      NO_INTR(rc = write(f, buf, len));
      if (rc < 0) {
        return rc;
      } else if (rc == 0) {
        break;
      }
      buf += rc;
      len -= rc;
    }
    return bytes - len;
}


struct WriterFds {
  size_t max_length;
  int    write_fd;
  int    compressed_fd;
  int    out_fd;
};

struct io {
  int fd;
  unsigned char *data, *end;
  unsigned char buf[4096];
};


/* Dynamically determines the byte sex of the system. Returns non-zero
 * for big-endian machines.
 */
static inline int sex() {
  int probe = 1;
  return !*(char *)&probe;
}


/* This function is invoked from a seperate process. It has access to a
 * copy-on-write copy of the parents address space, and all crucial
 * information about the parent has been computed by the caller.
 */
int CreateElfCore(char *fn, uint32_t ram_addr, uint8_t *raw_buf, uint32_t ram_size, Frame *frame)
{
  /* Count the number of mappings in "/proc/self/maps". We are guaranteed
   * that this number is not going to change while this function executes.
   */

  int handle;
  prpsinfo      prpsinfo;
  prstatus      prstatus;

  int num_threads = 1;
  int rc = 0;
  int num_mappings = 1;
  struct io io;
  struct {
      size_t start_address, end_address, offset;
      int   flags;
    } mappings[1] = {
            { (size_t)ram_addr, (size_t)ram_addr + ram_size, 0, PF_W|PF_R }
    };

    size_t note_align;
    int i;
    size_t pagesize = 4096;

  io.data = io.end = 0;

  handle = open(fn, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        /* Write out the ELF header                                          */
        /* scope */ {
          Ehdr ehdr;
          memset(&ehdr, 0, sizeof(Ehdr));
          ehdr.e_ident[0] = ELFMAG0;
          ehdr.e_ident[1] = ELFMAG1;
          ehdr.e_ident[2] = ELFMAG2;
          ehdr.e_ident[3] = ELFMAG3;
          ehdr.e_ident[4] = ELF_CLASS;
          ehdr.e_ident[5] = sex() ? ELFDATA2MSB : ELFDATA2LSB;
          ehdr.e_ident[6] = EV_CURRENT;
          ehdr.e_type     = ET_CORE;
          ehdr.e_machine  = ELF_ARCH;
          ehdr.e_version  = EV_CURRENT;
          ehdr.e_phoff    = sizeof(Ehdr);
          ehdr.e_ehsize   = sizeof(Ehdr);
          ehdr.e_phentsize= sizeof(Phdr);
          ehdr.e_phnum    = num_mappings + 1;
          ehdr.e_shentsize= sizeof(Shdr);
          if (c_write(handle, &ehdr, sizeof(Ehdr)) != sizeof(Ehdr)) {
            assert(0);
            goto done;
          }
        }

        /* Write program headers, starting with the PT_NOTE entry            */
        /* scope */
        {
          Phdr   phdr;
          size_t offset   = sizeof(Ehdr) + (num_mappings + 1)*sizeof(Phdr);
          size_t filesz   = sizeof(Nhdr) + 4 + sizeof(struct prpsinfo) +
                            num_threads*(
                            + sizeof(Nhdr) + 4 + sizeof(struct prstatus)
                            );

          memset(&phdr, 0, sizeof(Phdr));
          phdr.p_type     = PT_NOTE;
          phdr.p_offset   = offset;
          phdr.p_filesz   = filesz;
          if (c_write(handle, &phdr, sizeof(Phdr)) != sizeof(Phdr)) {
            assert(0);
            goto done;
          }

          /* Now follow with program headers for each of the memory segments */
          phdr.p_type     = PT_LOAD;
          phdr.p_align    = pagesize;
          phdr.p_paddr    = 0;
          note_align      = phdr.p_align - ((offset+filesz) % phdr.p_align);
          if (note_align == phdr.p_align)
            note_align    = 0;
          offset         += note_align;
          for (i = 0; i < num_mappings; i++) {
            offset       += filesz;
            filesz        = mappings[i].end_address -mappings[i].start_address;
            phdr.p_offset = offset;
            phdr.p_vaddr  = mappings[i].start_address;
            phdr.p_memsz  = filesz;

            /* Do not write contents for memory segments that are read-only  */
            if ((mappings[i].flags & PF_W) == 0)
              filesz      = 0;
            phdr.p_filesz = filesz;
            phdr.p_flags  = mappings[i].flags;
            if (c_write(handle, &phdr, sizeof(Phdr)) != sizeof(Phdr)) {
              assert(0);
              goto done;
            }
          }
        }

        /* Write note section                                                */
        /* scope */ {
          Nhdr nhdr;
          memset(&nhdr, 0, sizeof(Nhdr));
          nhdr.n_namesz   = 4;
          nhdr.n_descsz   = sizeof(struct prpsinfo);
          nhdr.n_type     = NT_PRPSINFO;
          if (c_write(handle, &nhdr, sizeof(Nhdr)) != sizeof(Nhdr) ||
              c_write(handle, "CORE", 4) != 4 ||
              c_write(handle, &prpsinfo, sizeof(struct prpsinfo)) !=
              sizeof(struct prpsinfo)) {
              assert(0);
            goto done;
          }

          for (i = num_threads; i-- > 0; ) {
            /* Process status and integer registers                          */
            nhdr.n_descsz = sizeof(struct prstatus);
            nhdr.n_type   = NT_PRSTATUS;
            prstatus.pr_pid = 1;
            //prstatus.pr_reg = regs[i];
            if (c_write(handle, &nhdr, sizeof(Nhdr)) != sizeof(Nhdr) ||
                c_write(handle, "CORE", 4) != 4 ||
                c_write(handle, &prstatus, sizeof(struct prstatus)) !=
                sizeof(struct prstatus)) {
                assert(0);
              goto done;
            }
          }
        }

        /* Align all following segments to multiples of page size            */
        if (note_align) {
          char scratch[note_align];
          memset(scratch, 0, note_align);
          if (c_write(handle, scratch, sizeof(scratch)) != sizeof(scratch)) {
            assert(0);
            goto done;
          }
        }

        /* Write all memory segments                                         */
        for (i = 0; i < num_mappings; i++) {
          if ((mappings[i].flags & PF_W) &&
              c_write(handle, (void *)raw_buf,
                     mappings[i].end_address - mappings[i].start_address) !=
                     mappings[i].end_address - mappings[i].start_address) {
            goto done;
          }
        }
        {
          Phdr   phdr;
          memset(&phdr, 0, sizeof(Phdr));
          phdr.p_type     = PT_NULL;
          phdr.p_paddr    = 0;
            if (c_write(handle, &phdr, sizeof(Phdr)) != sizeof(Phdr)) {
              assert(0);
              goto done;
            }
        }
done:
    close(handle);
    return rc;
}

