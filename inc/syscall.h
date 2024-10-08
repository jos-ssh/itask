#ifndef JOS_INC_SYSCALL_H
#define JOS_INC_SYSCALL_H

/* system call numbers */
enum {
    SYS_cputs = 0,
    SYS_cgetc,
    SYS_getenvid,
    SYS_env_destroy,
    SYS_alloc_region,
    SYS_map_region,
    SYS_map_physical_region,
    SYS_unmap_region,
    SYS_region_refs,
    SYS_exofork,
    SYS_env_set_status,
    SYS_env_set_trapframe,
    SYS_env_set_pgfault_upcall,
    SYS_env_set_parent,
    SYS_env_downgrade,
    SYS_yield,
    SYS_ipc_try_send,
    SYS_ipc_recv,
    SYS_gettime,
    SYS_get_rsdp_paddr,
    SYS_crypto, 
    SYS_crypto_get, 
    NSYSCALLS
};

#endif /* !JOS_INC_SYSCALL_H */
