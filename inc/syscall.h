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
    SYS_unmap_region,
    SYS_region_refs,
    SYS_exofork,
    SYS_env_set_status,
    SYS_env_set_trapframe,
    SYS_env_set_pgfault_upcall,
    SYS_yield,
    SYS_ipc_try_send,
    SYS_ipc_recv,
    SYS_gettime,
    SYS_setuid,
    SYS_getuid,
    SYS_setgid,
    SYS_getgid,
    SYS_seteuid,
    SYS_geteuid,
    SYS_setegid,
    SYS_getegid,
    SYS_getenvcurpath,
    SYS_setenvcurpath,
    NSYSCALLS
};

#endif /* !JOS_INC_SYSCALL_H */
