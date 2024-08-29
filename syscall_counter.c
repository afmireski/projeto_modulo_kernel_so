/**
 * Módulo de Contador de Syscalls
 * Autores: André Felipe Mireski, Alexandre Tolomeotti, Victor Ângelo Souza Santos
 */

#include <linux/kernel.h>     // Para funções do kernel
#include <linux/module.h>     // Necessário para todos os módulos
#include <linux/proc_fs.h>    // Para criar arquivos no /proc
#include <linux/uaccess.h>    // Para copiar dados para o espaço do usuário
#include <linux/sched.h>      // Para obter informações sobre processos
#include <linux/syscalls.h>   // Para interceptar syscalls

// MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Módulo de Contador de Syscalls");
MODULE_VERSION("0.1");

#define PROC_NAME "syscall_counter"
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
static struct file_operations proc_fops = {
    .owner = THIS_MODULE,
    .read = proc_read,
};

// Função para interceptar uma syscall (exemplo com sys_open)
asmlinkage long my_sys_open(const char __user *filename, int flags, mode_t mode) {
    syscall_count++;
    return sys_open(filename, flags, mode); // Chama a syscall original
}

// Função de inicialização do módulo
static int __init syscall_counter_init(void) {
    // Cria o arquivo /proc/syscall_counter
    proc_file = proc_create(PROC_NAME, 0, NULL, &proc_fops);
    if (!proc_file) {
        return -ENOMEM;
    }
    
    printk(KERN_INFO "Syscall Counter Module Loaded\n");
    return 0;
}

// Função de saída do módulo
static void __exit syscall_counter_exit(void) {
    // Remove o arquivo /proc/syscall_counter
    proc_remove(proc_file);
    printk(KERN_INFO "Syscall Counter Module Unloaded\n");
}

module_init(syscall_counter_init);
module_exit(syscall_counter_exit);
