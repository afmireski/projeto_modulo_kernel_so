#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/uaccess.h> // Para funções de manipulação de memória

static struct kprobe kp = {
    .symbol_name = "__x64_sys_newuname",  // Nome da função que queremos monitorar
};

// Função que será chamada quando a função `sys_newuname` for interceptada
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    const char __user *filename = (const char __user *)regs->si; // Argumento 1 de sys_newuname
    char fname[256];
    long ret;

    // Copia o nome do arquivo do espaço do usuário para o kernel
    ret = strncpy_from_user(fname, filename, sizeof(fname) - 1);
    if (ret > 0) {
        fname[ret] = '\0';
        printk(KERN_INFO "sys_newuname chamada com filename: %s\n", fname);
    } else {
        printk(KERN_INFO "sys_newuname chamada com filename não acessível\n");
    }

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
    return 0;
}

// Função de saída do módulo
static void __exit kprobe_exit(void)
{
    unregister_kprobe(&kp);
    printk(KERN_INFO "kprobe desregistrada\n");
}

module_init(kprobe_init);
module_exit(kprobe_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Módulo de Contador de Syscalls usando kprobes para sys_newuname");
MODULE_VERSION("0.1");
