// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/ktime.h>
#include <linux/cpufreq.h>
#include <linux/sched/cputime.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/capability.h>

struct sys_resource_usage {
    unsigned long ram_total_kb;
    unsigned long ram_free_kb;
    unsigned int ram_usage_percent;
    unsigned int cpu_usage_percent;
};

/* Variables estáticas para medir uso de CPU */
static u64 prev_idle = 0, prev_total = 0;

/* =======================================================
 * FUNCIÓN AUXILIAR: CÁLCULO DE USO DE CPU
 * ======================================================= */
static unsigned int get_cpu_usage(void)
{
    u64 user, nice, system, idle, iowait, irq, softirq, steal;
    u64 total, diff_idle, diff_total;
    unsigned int usage = 0;
    struct file *f;
    char buf[256];
    loff_t pos = 0;
    ssize_t n;

    f = filp_open("/proc/stat", O_RDONLY, 0);
    if (IS_ERR(f))
        return 0;

    n = kernel_read(f, buf, sizeof(buf) - 1, &pos);
    filp_close(f, NULL);
    if (n <= 0)
        return 0;

    buf[n] = '\0';
    sscanf(buf, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);

    total = user + nice + system + idle + iowait + irq + softirq + steal;
    diff_idle = idle - prev_idle;
    diff_total = total - prev_total;

    if (diff_total > 0)
        usage = (100 * (diff_total - diff_idle)) / diff_total;

    prev_total = total;
    prev_idle = idle;

    return usage;
}

/* =======================================================
 * SYSCALL: obtener uso de CPU y RAM
 * ======================================================= */
SYSCALL_DEFINE1(get_system_usage, struct sys_resource_usage __user *, info)
{
    struct sysinfo si;
    struct sys_resource_usage data;
    unsigned long used_kb;

    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    si_meminfo(&si);

    data.ram_total_kb = (si.totalram * si.mem_unit) / 1024;
    data.ram_free_kb = (si.freeram * si.mem_unit) / 1024;
    used_kb = data.ram_total_kb - data.ram_free_kb;

    data.ram_usage_percent = (data.ram_total_kb > 0)
                                 ? (used_kb * 100) / data.ram_total_kb
                                 : 0;

    data.cpu_usage_percent = get_cpu_usage();

    if (copy_to_user(info, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Syscall para obtener uso de CPU y RAM (USACLinux)");
