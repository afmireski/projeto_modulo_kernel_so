/**
 * Módulo de Contador de Syscalls
 * Autores: André Felipe Mireski, Alexandre Tolomeotti, Victor Ângelo Souza Santos
 */

#include <linux/kernel.h>   // Para funções do kernel
#include <linux/module.h>   // Necessário para todos os módulos
#include <linux/proc_fs.h>  // Para criar arquivos no /proc
#include <linux/uaccess.h>  // Para copiar dados para o espaço do usuário
#include <linux/sched.h>    // Para obter informações sobre processos
#include <linux/syscalls.h> // Para interceptar syscalls
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/err.h>
#include <linux/slab.h>     // Para kmalloc e kfree

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Módulo de Contador de Syscalls");
MODULE_VERSION("0.1");

#define PROC_NAME "syscall_counter"
static struct proc_dir_entry *proc_file;

// Contador de syscalls
static unsigned long syscall_count = 0;

// Função de callback que será chamada quando o arquivo /proc/syscall_counter for lido
static ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    char buffer[64];
    int len = sprintf(buffer, "Syscall count: %lu\n", syscall_count);

    return simple_read_from_buffer(buf, count, pos, buffer, len);
}

// Estrutura de file_operations para mapear a função de leitura
static struct proc_ops proc_fops = {
    .proc_read = proc_read,
};

// Função para interceptar uma syscall (exemplo com sys_open)
asmlinkage long my_sys_open(const char __user *filename, int flags, mode_t mode)
{
    struct file *file;
    long err = 0;

    // Contabilizar a chamada
    syscall_count++;
    printk("Interceptação ocorreu\n");

    // Alocar memória para o caminho do arquivo (buffer de kernel)
    char *kernel_filename = kmalloc(PATH_MAX, GFP_KERNEL);
    if (!kernel_filename)
    {
        printk(KERN_ERR "Falha ao alocar memória para filename\n");
        return -ENOMEM;
    }

    // Copiar o nome do arquivo do espaço de usuário para o espaço do kernel
    if (copy_from_user(kernel_filename, filename, PATH_MAX))
    {
        kfree(kernel_filename);
        printk(KERN_ERR "Falha ao copiar o nome do arquivo do espaço de usuário\n");
        return -EFAULT;
    }

    // Tentar abrir o arquivo no kernel sem alterar o segmento de memória
    file = filp_open(kernel_filename, flags, mode);
    if (IS_ERR(file))
    {
        err = PTR_ERR(file);
        printk(KERN_ERR "Falha ao abrir arquivo: %ld\n", err);
    }
    else
    {
        // Sucesso: Agora você pode fazer outras operações como vfs_read(), vfs_write(), etc.
        filp_close(file, NULL);
    }

    kfree(kernel_filename); // Liberar memória alocada

    return err;
}

// Função de inicialização do módulo
static int __init syscall_counter_init(void)
{
    // Cria o arquivo /proc/syscall_counter
    proc_file = proc_create(PROC_NAME, 0, NULL, &proc_fops);
    if (!proc_file)
    {
        return -ENOMEM;
    }

    printk(KERN_INFO "Syscall Counter Module Loaded\n");
    return 0;
}

// Função de saída do módulo
static void __exit syscall_counter_exit(void)
{
    // Remove o arquivo /proc/syscall_counter
    proc_remove(proc_file);
    printk(KERN_INFO "Syscall Counter Module Unloaded\n");
}

module_init(syscall_counter_init);
module_exit(syscall_counter_exit);
