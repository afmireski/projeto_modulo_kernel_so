/**
 * Módulo de Contador de Syscalls
 * Autores: André Felipe Mireski, Alexandre Tolomeotti, Victor Ângelo Souza Santos
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/uaccess.h> // Para funções de manipulação de memória
#include <linux/proc_fs.h> // Para criar arquivos no /proc
#include <linux/sched.h>   // Para obter informações sobre processos

#define PROC_NAME "scm_syscall_counter"
#define TARGET_PROC "scm_target_syscall"
#define DEFAULT_SYSCALL "__x64_sys_newuname"

static struct proc_dir_entry *proc_file, *target_file;
static unsigned long syscall_count = 0;                // Contador de syscalls
static char target_syscall[64] = DEFAULT_SYSCALL; // Nome da syscall a ser monitorada
static struct kprobe kp = {
    .symbol_name = DEFAULT_SYSCALL,
}; // Estrutura do kprobe

// Função de callback para o arquivo /proc/syscall_counter
static ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    char buffer[64];
    int len = sprintf(buffer, "Syscall count: %lu\n", syscall_count);
    if (len == 0) {
        printk(KERN_INFO "Nada escrito no contador\n");
        return 0;
    } else if (len < 0) {
        printk(KERN_INFO "Escrita inválida no contador\n");
        return -EINVAL;
    }
    return simple_read_from_buffer(buf, count, pos, buffer, len);
}

// Função de leitura do arquivo /proc/target
static ssize_t target_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    return simple_read_from_buffer(buf, count, pos, target_syscall, strlen(target_syscall));
}

// Função que será chamada quando a syscall monitorada for interceptada
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    printk(KERN_INFO "%s interceptada!\n", p->symbol_name);
    syscall_count++;
    return 0;
}

// Função de escrita no arquivo /proc/target
static ssize_t target_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
    int ret;

    if (count > sizeof(target_syscall) - 1)
    {

        printk(KERN_INFO "Target muito grande\n");
        return -EINVAL;
    }

    if (copy_from_user(target_syscall, buf, count))
    {
        printk(KERN_INFO "Falha na cópia\n");
        return -EFAULT;
    }

    target_syscall[count] = '\0'; // Adiciona o terminador de string

    // Remove o kprobe anterior, se houver
    unregister_kprobe(&kp);
    printk(KERN_INFO "Antiga syscall: %s\n", kp.symbol_name);

    // Registra o novo kprobe com o símbolo definido
    printk(KERN_INFO "Nova syscall: %s|%lu", target_syscall, count);

    struct kprobe kp_temp = {
        .symbol_name = target_syscall,
        .pre_handler = handler_pre,
    };
    syscall_count = 0;

    kp = kp_temp;

    ret = register_kprobe(&kp);
    if (ret < 0)
    {
        printk(KERN_ERR "Erro ao registrar kprobe para syscall 1: %s\n", kp.symbol_name);
        return ret;
    }

    printk(KERN_INFO "kprobe registrado para syscall 1: %s\n", kp.symbol_name);
    return count;
}

// Estruturas para mapear as operações dos arquivos /proc
static struct proc_ops proc_fops = {
    .proc_read = proc_read,
};

static struct proc_ops target_fops = {
    .proc_read = target_read,
    .proc_write = target_write,
};

// Função de inicialização do módulo
static int __init kprobe_init(void)
{
    kp.pre_handler = handler_pre;

    int ret = register_kprobe(&kp);
    if (ret < 0)
    {
        printk(KERN_ERR "Erro ao registrar kprobe para syscall 2: %s\n", target_syscall);
        return ret;
    }

    printk(KERN_INFO "kprobe registrado para syscall 2: %s\n", target_syscall);

    // Cria o arquivo /proc/syscall_counter
    proc_file = proc_create(PROC_NAME, 0, NULL, &proc_fops);
    if (!proc_file)
    {
        printk(KERN_ERR "Proc file failed\n");
        return -ENOMEM;
    }

    // Cria o arquivo /proc/target para definir a syscall monitorada
    target_file = proc_create(TARGET_PROC, 0666, NULL, &target_fops);
    if (!target_file)
    {
        printk(KERN_ERR "Target file failed\n");
        proc_remove(proc_file);
        return -ENOMEM;
    }
    
    printk(KERN_INFO "Syscall Counter Module Loaded\n");
    return 0;
}

// Função de saída do módulo
static void __exit kprobe_exit(void)
{
    unregister_kprobe(&kp);
    if (proc_file) {
        printk(KERN_INFO "Removendo proc file");
        proc_remove(proc_file);
    }
    if (target_file) {
        printk(KERN_INFO "Removendo target file");
        proc_remove(target_file);
    }
    printk(KERN_INFO "Syscall Counter Module Unloaded\n");
}

module_init(kprobe_init);
module_exit(kprobe_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Módulo de Contador de Syscalls usando kprobes");
MODULE_VERSION("0.1");
