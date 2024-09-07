/**
 * Módulo de Contador de Syscalls
 * Autores: André Felipe Mireski, Alexandre Tolomeotti, Victor Ângelo Souza Santos
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h> // Para kprobes e suas funções de interceptação
#include <linux/uaccess.h> // Para funções de manipulação de memória
#include <linux/proc_fs.h> // Para criar arquivos no /proc
#include <linux/sched.h>   // Para obter informações sobre processos

#define PROC_NAME "syscall_counter" // Nome do arquivo onde ficará registrado o nº de chamadas
#define TARGET_SYSCALL "__x64_sys_newuname"
static struct proc_dir_entry *proc_file;

// Contador de chamadas da syscall
static unsigned long syscall_count = 0;

/**
 * Função de callback que será chamada quando o arquivo /proc/syscall_counter for lido
 * @param file Ponteiro para o arquivo
 * @param buf Buffer de escrita
 * @param count Tamanho do buffer
 * @param pos Offset do buffer
 */
static ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    char buffer[64];
    int len = sprintf(buffer, "%s chamada %lu vezes.\n", TARGET_SYSCALL, syscall_count);

    return simple_read_from_buffer(buf, count, pos, buffer, len);
}

// Estrutura de file_operations para mapear a função de leitura
static struct proc_ops proc_fops = {
    .proc_read = proc_read,
};

// Estrutura de kprobe
static struct kprobe kp = {
    .symbol_name = TARGET_SYSCALL, // Nome da função que queremos monitorar
};

/**
 * Função que será chamada quando a syscall for interceptada
 * @param p Ponteiro para o kprobe
 * @param regs Ponteiro para os registradores
 */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{

    printk(KERN_INFO "%s interceptada.\n", TARGET_SYSCALL, syscall_count);
    syscall_count++;

    return 0;
}


/**
 * Função que será chamada na inicialização do módulo, registrando o kprobe e criando o arquivo /proc/syscall_counter
 */
static int __init kprobe_init(void)
{
    int ret;
    kp.pre_handler = handler_pre; // Define a função de interceptação

    // Registrar o kprobe
    ret = register_kprobe(&kp);
    if (ret < 0) // Verifica se o kprobe foi registrado
    {
        printk(KERN_ERR "Erro ao registrar kprobe: %d\n", ret);
        return ret;
    }

    printk(KERN_INFO "kprobe registrada para %s\n", TARGET_SYSCALL);

    // Cria o arquivo /proc/syscall_counter
    proc_file = proc_create(PROC_NAME, 0, NULL, &proc_fops);
    if (!proc_file) // Se não criou o arquivo, retorna erro
    {
        printk(KERN_ERR "Erro ao criar o arquivo /proc/syscall_counter\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "Syscall Counter Module Carregado\n");
    return 0;
}
/**
 * Função que será chamada no descarregamento do módulo, removendo o kprobe e removendo o arquivo /proc/syscall_counter
 */
static void __exit kprobe_exit(void)
{
    unregister_kprobe(&kp);
    printk(KERN_INFO "kprobe desvinculado\n");
    proc_remove(proc_file);
    printk(KERN_INFO "Syscall Counter Module Descarregado\n");
}

module_init(kprobe_init);
module_exit(kprobe_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Módulo de Contador de Syscalls usando kprobes");
MODULE_VERSION("0.1");
