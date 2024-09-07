/**
 * Módulo de Contador de Syscalls
 * Autores: André Felipe Mireski, Alexandre Tolomeotti, Victor Ângelo Souza Santos
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/uaccess.h> // Para funções de manipulação de memória
#include <linux/proc_fs.h>    // Para criar arquivos no /proc
#include <linux/sched.h>      // Para obter informações sobre processos

#define PROC_NAME "sys_newuname_counter"
static struct proc_dir_entry *proc_file;

// Contador de syscalls
static unsigned long syscall_count = 0;

// Função de callback que será chamada quando o arquivo /proc/syscall_counter for lido
static ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *pos) {
    char buffer[64];
    int len = sprintf(buffer, "Syscall count: %lu\n", syscall_count);
    
    return simple_read_from_buffer(buf, count, pos, buffer, len);
}

// Estrutura de file_operations para mapear a função de leitura
static struct proc_ops proc_fops = {
    .proc_read = proc_read,
};

static struct kprobe kp = {
    .symbol_name = "__x64_sys_newuname",  // Nome da função que queremos monitorar
};

// Função que será chamada quando a função `sys_newuname` for interceptada
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    
    syscall_count++;

    return 0;
}

// Função de inicialização do módulo
static int __init kprobe_init(void)
{
    int ret;
    kp.pre_handler = handler_pre;

    // Registrar o kprobe
    ret = register_kprobe(&kp);
    if (ret < 0) {
        printk(KERN_ERR "Erro ao registrar kprobe: %d\n", ret);
        return ret;
    }

    printk(KERN_INFO "kprobe registrada para sys_newuname\n");

    // Cria o arquivo /proc/syscall_counter
    proc_file = proc_create(PROC_NAME, 0, NULL, &proc_fops);
    if (!proc_file) {
        return -ENOMEM;
    }
    
    printk(KERN_INFO "Syscall Counter Module Loaded\n");
    return 0;
}

// Função de saída do módulo
static void __exit kprobe_exit(void)
{
    unregister_kprobe(&kp);
    printk(KERN_INFO "kprobe desregistrada\n");
    proc_remove(proc_file);
    printk(KERN_INFO "Syscall Counter Module Unloaded\n");
}

module_init(kprobe_init);
module_exit(kprobe_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Módulo de Contador de Syscalls usando kprobes para sys_newuname");
MODULE_VERSION("0.1");
