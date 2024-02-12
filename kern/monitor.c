/* Simple command-line kernel monitor useful for
 * controlling the kernel and exploring the system interactively. */

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/env.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/cpu.h>
#include <kern/monitor.h>
#include <kern/kclock.h>
#include <kern/kdebug.h>
#include <kern/tsc.h>
#include <kern/timer.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>

#define WHITESPACE "\t\r\n "
#define MAXARGS    16

/* Functions implementing monitor commands */
int mon_help(int argc, char **argv, struct Trapframe *tf);
int mon_kerninfo(int argc, char **argv, struct Trapframe *tf);
int mon_backtrace(int argc, char **argv, struct Trapframe *tf);
int mon_echo(int argc, char **argv, struct Trapframe *tf);
int mon_halt(int argc, char **argv, struct Trapframe *tf);
int mon_dumpcmos(int argc, char **argv, struct Trapframe *tf);
int mon_start(int argc, char **argv, struct Trapframe *tf);
int mon_stop(int argc, char **argv, struct Trapframe *tf);
int mon_frequency(int argc, char **argv, struct Trapframe *tf);
int mon_memory(int argc, char **argv, struct Trapframe *tf);
int mon_pagetable(int argc, char **argv, struct Trapframe *tf);
int mon_virt(int argc, char **argv, struct Trapframe *tf);

struct Command {
    const char *name;
    const char *desc;
    /* return -1 to force monitor to exit */
    int (*func)(int argc, char **argv, struct Trapframe *tf);
};

static struct Command commands[] = {
        {"help", "Display this list of commands", mon_help},
        {"kerninfo", "Display information about the kernel", mon_kerninfo},
        {"backtrace", "Print stack backtrace", mon_backtrace},
        {"echo", "Print arguments into command-line", mon_echo},
        {"halt", "Halt the processor", mon_halt},
        {"dumpcmos", "Display CMOS contents", mon_dumpcmos},
        {"timer_start", "Start timer", mon_start},
        {"timer_stop", "Stop timer", mon_stop},
        {"timer_freq", "Get timer frequency", mon_frequency},
        {"memory", "Display allocated memory pages", mon_memory},
        {"pagetable", "Display current page table", mon_pagetable},
        {"virt", "Display virtual memory tree", mon_virt},
};
#define NCOMMANDS (sizeof(commands) / sizeof(commands[0]))

/* Implementations of basic kernel monitor commands */

int
mon_help(int argc, char **argv, struct Trapframe *tf) {
    for (size_t i = 0; i < NCOMMANDS; i++)
        cprintf("%s - %s\n", commands[i].name, commands[i].desc);
    return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf) {
    extern char _head64[], entry[], etext[], edata[], end[];

    cprintf("Special kernel symbols:\n");
    cprintf("  _head64 %16lx (virt)  %16lx (phys)\n", (unsigned long)_head64, (unsigned long)_head64);
    cprintf("  entry   %16lx (virt)  %16lx (phys)\n", (unsigned long)entry, (unsigned long)entry - KERN_BASE_ADDR);
    cprintf("  etext   %16lx (virt)  %16lx (phys)\n", (unsigned long)etext, (unsigned long)etext - KERN_BASE_ADDR);
    cprintf("  edata   %16lx (virt)  %16lx (phys)\n", (unsigned long)edata, (unsigned long)edata - KERN_BASE_ADDR);
    cprintf("  end     %16lx (virt)  %16lx (phys)\n", (unsigned long)end, (unsigned long)end - KERN_BASE_ADDR);
    cprintf("Kernel executable memory footprint: %luKB\n", (unsigned long)ROUNDUP(end - entry, 1024) / 1024);
    return 0;
}

int
mon_echo(int argc, char **argv, struct Trapframe *tf) {
    if (argc < 2)
        return 0;

    cprintf("%s", argv[1]);
    for (int arg_idx = 2; arg_idx < argc; ++arg_idx) {
        cprintf(" %s", argv[arg_idx]);
    }
    cprintf("\n");

    return 0;
}


int
mon_backtrace(int argc, char **argv, struct Trapframe *tf) {
    unsigned long rbp = read_rbp();
    unsigned long rip = read_rip();
    struct Ripdebuginfo debug_info;

    cprintf("Stack backtrace:\n");
    while (rbp) {
        unsigned long *base_ptr = (unsigned long *)rbp;
        debuginfo_rip(rip, &debug_info);

        cprintf("  rbp %016lx  rip %016lx\n", rbp, rip);
        cprintf("    %.*s:%d: %.*s+%lu\n",
                RIPDEBUG_BUFSIZ, debug_info.rip_file, debug_info.rip_line,
                debug_info.rip_fn_namelen, debug_info.rip_fn_name,
                rip - 5 - debug_info.rip_fn_addr);

        rbp = base_ptr[0];
        rip = base_ptr[1];
    }

    return 0;
}

_Noreturn int
mon_halt(int argc, char **argv, struct Trapframe *tf) {
    asm volatile(
            "movq $0, %%rbp\n"
            "movq %0, %%rsp\n"
            "pushq $0\n"
            "pushq $0\n"
            "sti\n"
            "hlt\n" ::"a"(cpu_ts.ts_rsp0));

    /* Unreachable */
    for (;;)
        ;
}

/* Implement timer_start (mon_start), timer_stop (mon_stop), timer_freq (mon_frequency) commands. */
// LAB 5: Your code here:

int
mon_start(int argc, char **argv, struct Trapframe *tf) {
    if (argc == 1) {
      /* Use default timer */
      timer_start(timer_for_schedule->timer_name);
    }
    else if (argc == 2) {
      timer_start(argv[1]);
    }
    else {
      cprintf("Usage:\n%s [TIMER]\n", argv[0]);
      return 1;
    }
    return 0;
}

int
mon_stop(int argc, char **argv, struct Trapframe *tf) {
    timer_stop();
    return 0;
}

int
mon_frequency(int argc, char **argv, struct Trapframe *tf) {
    if (argc == 1) {
      /* Use default timer */
      timer_cpu_frequency(timer_for_schedule->timer_name);
    }
    else if (argc == 2) {
      timer_cpu_frequency(argv[1]);
    }
    else {
      cprintf("Usage:\n%s [TIMER]\n", argv[0]);
      return 1;
    }
    return 0;
}

/* Implement memory (mon_memory) commands. */
int
mon_memory(int argc, char **argv, struct Trapframe *tf) {
    dump_memory_lists();
    return 0;
}

/* Implement mon_pagetable() and mon_virt()
 * (using dump_virtual_tree(), dump_page_table())*/
int
mon_pagetable(int argc, char **argv, struct Trapframe *tf) {
    dump_page_table(current_space->pml4);
    return 0;
}

int
mon_virt(int argc, char **argv, struct Trapframe *tf) {
    dump_virtual_tree(current_space->root, MAX_CLASS);
    return 0;
}

int
mon_dumpcmos(int argc, char **argv, struct Trapframe *tf) {
    // Dump CMOS memory in the following format:
    // 00: 00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF
    // 10: 00 ..
    // Make sure you understand the values read.
    // Hint: Use cmos_read8()/cmos_write8() functions.

    // For each address in CMOS memory
    for (unsigned addr = 0; addr <= CMOS_MAX_ADDR; ++addr) {
        // If address is divisible by 16
        if (addr % 0x10 == 0) {
            // If address is non-zero
            if (addr) {
                // Begin new line
                cputchar('\n');
            }
            // Print address
            cprintf("%02x:", addr);
        }
        // Print data at address
        cprintf(" %02x", cmos_read8(addr));
    }
    cputchar('\n');
    return 0;
}

/* Kernel monitor command interpreter */

static int
runcmd(char *buf, struct Trapframe *tf) {
    int argc = 0;
    char *argv[MAXARGS];

    argv[0] = NULL;

    /* Parse the command buffer into whitespace-separated arguments */
    for (;;) {
        /* gobble whitespace */
        while (*buf && strchr(WHITESPACE, *buf)) *buf++ = 0;
        if (!*buf) break;

        /* save and scan past next arg */
        if (argc == MAXARGS - 1) {
            cprintf("Too many arguments (max %d)\n", MAXARGS);
            return 0;
        }
        argv[argc++] = buf;
        while (*buf && !strchr(WHITESPACE, *buf)) buf++;
    }
    argv[argc] = NULL;

    /* Lookup and invoke the command */
    if (!argc) return 0;
    for (size_t i = 0; i < NCOMMANDS; i++) {
        if (strcmp(argv[0], commands[i].name) == 0)
            return commands[i].func(argc, argv, tf);
    }

    cprintf("Unknown command '%s'\n", argv[0]);
    return 0;
}

void
monitor(struct Trapframe *tf) {

    cprintf("Welcome to the JOS kernel monitor!\n");
    cprintf("Type 'help' for a list of commands.\n");

    if (tf) print_trapframe(tf);

    char *buf;
    do
        buf = readline("K> ");
    while (!buf || runcmd(buf, tf) >= 0);
}
