#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/compiler.h>
#include <linux/sched/signal.h>

/*TODO: do this properly*/
/*#include <uapi/asm-generic/unistd.h>*/
#ifndef __NR_syscalls
# define __NR_syscalls (__NR_supersyscall+1)
#endif

#define uif(Cond)  if(unlikely(Cond))
#define lif(Cond)  if(likely(Cond))
 

typedef asmlinkage long (*sys_call_ptr_t)(unsigned long, unsigned long,
					  unsigned long, unsigned long,
					  unsigned long, unsigned long);
extern const sys_call_ptr_t sys_call_table[];

static bool 
syscall__failed(unsigned long Ret)
{
   return (Ret > -4096UL);
}


static bool
syscall(unsigned Nr, long A[6])
{
    uif (Nr >= __NR_syscalls )
        return -ENOSYS;
    return sys_call_table[Nr](A[0], A[1], A[2], A[3], A[4], A[5]);
}


static int 
segfault(void const *Addr)
{
    struct siginfo info[1];
    info->si_signo = SIGSEGV;
    info->si_errno = 0;
    info->si_code = 0;
    info->si_addr = (void*)Addr;
    return send_sig_info(SIGSEGV, info, current);
    //return force_sigsegv(SIGSEGV, current);
}

asmlinkage long /*Ntried*/
sys_supersyscall(long* Rets, 
                 struct supersyscall_args *Args, 
                 int Nargs, 
                 int Flags)
{
    int i = 0, nfinished = 0;
    struct supersyscall_args args; /*7 * sizeof(long) */
    
    for (i = 0; i<Nargs; i++){
        long ret;

        uif (0!=copy_from_user(&args, Args+i, sizeof(args))){
            segfault(&Args+i);
            return nfinished;
        }

        ret = syscall(args.call_nr, args.args);
        nfinished++;

        if ((Flags & 1) == SUPERSYSCALL__abort_on_failure 
                &&  syscall__failed(ret))
            return nfinished;


        uif (0!=put_user(ret, Rets+1)){
            segfault(Rets+i);
            return nfinished;
        }
    }
    return nfinished;

}


