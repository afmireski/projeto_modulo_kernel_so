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

#define COUNTER_PROC "scm_syscall_counter" // Syscall que armazenará o valor do contador da syscall
#define TARGET_PROC "scm_target_syscall" // Arquivo que controlará qual syscall é monitorada
#define DEFAULT_SYSCALL "__x64_sys_newuname" // Syscall padrão que será monitorada

static struct proc_dir_entry *counter_file, *target_file;
static unsigned long syscall_count = 0;           // Contador de syscalls
static char target_syscall[64] = DEFAULT_SYSCALL; // Nome da syscall a ser monitorada
static struct kprobe kp = {
    .symbol_name = DEFAULT_SYSCALL, // o Symbol define o que o kprobe vai monitorar
}; // Estrutura do kprobe

/**
 * Função de callback que será chamada quando algum usuário realizar a leitura
 * do arquivo COUNTER_PROC, exportando o valor do contador.
 * 
 * @param file ponteiro para o arquivo
 * @param buf ponteiro para o buffer de escrita no espaço do usuário
 * @param count tamanho máximo do buffer que pode ser copiado
 * @param pos posição atual do buffer dentro do arquivo
 */
static ssize_t counter_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    char buffer[64];
    int len = sprintf(buffer, "%s chamada %lu vezes\n", target_syscall, syscall_count);
    if (len == 0)
    {
        printk(KERN_INFO "Nada escrito no contador\n");
        return 0;
    }
    else if (len < 0)
    {
        printk(KERN_INFO "Escrita inválida no contador\n");
        return -EINVAL;
    }
    // Escreve o buffer no arquivo /proc quando ele for lido
    return simple_read_from_buffer(buf, count, pos, buffer, len);
}


/**
 * Função de callback que será chamada quando algum usuário realizar a leitura
 * do arquivo TARGET_PROC, exportando a informacão da syscall de qual syscall está sendo monitorada.
 * 
 * @param file ponteiro para o arquivo
 * @param buf ponteiro para o buffer de escrita no espaço do usuário
 * @param count tamanho máximo do buffer que pode ser copiado
 * @param pos posição atual do buffer dentro do arquivo
 */
static ssize_t target_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    // Escreve o buffer no arquivo /proc quando ele for lido
    return simple_read_from_buffer(buf, count, pos, target_syscall, strlen(target_syscall));
}

/**
 * Função que será chamada quando o kprobe detectar que a syscall escolhida foi chamada.
 * @param p ponteiro para uma struct kprobe
 * @param regs estrutura que armazena o estado dos registradores da CPU
 */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    printk(KERN_INFO "%s interceptada!\n", p->symbol_name);
    syscall_count++;
    return 0;
}

/**
 * Função de callback que será chamada quando algum usuário realizar a escrita no arquivo TARGET_PROC
 * @param file ponteiro para o arquivo
 * @param buf ponteiro para o buffer de escrita no espaço do usuário
 * @param count tamanho máximo do buffer que pode ser copiado
 * @param pos posição atual do buffer dentro do arquivo
 */
static ssize_t target_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
    // Verifica se o conteúdo que está sendo escrito não é maior do que o tamanho do buffer suportado para armazenar o nome da syscall
    if (count > sizeof(target_syscall) - 1)
    {
        printk(KERN_INFO "Target muito grande\n");
        return -EINVAL;
    }

    // Copia o conteúdo do buffer no espaço de usuário para target_syscall
    if (copy_from_user(target_syscall, buf, count))
    {
        printk(KERN_INFO "Falha na cópia\n");
        return -EFAULT;
    }

    target_syscall[count] = '\0'; // Adiciona o terminador de string

    // Remove o kprobe anterior, se houver
    unregister_kprobe(&kp);

    // Cria um novo kprove
    // Somente atribuir valores para o kprobe existente
    // não funcionou.
    // Essa é a maneira correta de se inicializar um kprobe
    struct kprobe kp_temp = {
        .symbol_name = target_syscall, // Passa a nova syscall que será monitorada
        .pre_handler = handler_pre, // Passa a função de tratamento que será chamada quando o kprobe interceptar a syscall
    };
    syscall_count = 0; // Zera o contador para a nova syscall

    kp = kp_temp; // Copia o kp local para o global

    // Registra o novo kprobe com o símbolo informado no arquivo pelo usuário
    int ret = register_kprobe(&kp);

    // Verifica se o novo kprobe foi registrado
    if (ret < 0)
    {
        printk(KERN_ERR "Erro ao registrar kprobe para syscall: %s\n", kp.symbol_name);
        return ret;
    }

    printk(KERN_INFO "kprobe registrado para syscall: %s\n", kp.symbol_name);
    return count; // Devolve quantos caracteres foram escritos.
}

// Estrutura para mapear as operações que arquivos /proc podem realizar
// Operações do arquivo COUNTER_PROC
static struct proc_ops counter_proc_ops = {
    .proc_read = counter_read, // Define a operação que é chamada durante a leitura do arquivo
};

// Estrutura para mapear as operações que arquivos /proc podem realizar
// Operações do arquivo TARGET_PROC
static struct proc_ops target_proc_ops = {
    .proc_read = target_read, // Define a operação que é chamada durante a leitura do arquivo
    .proc_write = target_write, // Define a operação que é chamada durante a escrita no arquivo
};

/**
 * Função que é chamada ao carregar o módulo, registrando o kprobe default
 * e criando os arquivos TARGET_PROC e COUNTER_PROC
 */
static int __init syscall_counter_init(void)
{
    /// REGISTRO DO KPROBE
    //
    kp.pre_handler = handler_pre; // Passa a função de tratamento que será chamada quando o kprobe interceptar a syscall

    // Registra o kprobe
    int ret = register_kprobe(&kp);

    // Verifica se o kprobe foi registrado
    if (ret < 0)
    {
        printk(KERN_ERR "Erro ao registrar kprobe para syscall: %s\n", target_syscall);
        return ret;
    }

    printk(KERN_INFO "kprobe registrado para syscall: %s\n", target_syscall);

    /// --------

    // Cria o arquivo COUNTER_PROC para armazenar o valor do contador
    counter_file = proc_create(COUNTER_PROC, 0, NULL, &counter_proc_ops);
    if (!counter_file) // Valida se o arquivo foi criado
    {
        printk(KERN_ERR "Falha ao criar o COUNTER_PROC file\n");
        return -ENOMEM;
    }

    // Cria o arquivo TARGET_PROC que será utilizado para definir a syscall monitorada
    target_file = proc_create(TARGET_PROC, 0666, NULL, &target_proc_ops);
    if (!target_file) // Valida se o arquivo foi criado
    {
        printk(KERN_ERR "Falha ao criar o TARGET_PROC file\n");
        proc_remove(counter_file); // Remove o arquivo COUNTER_PROC caso o arquivo TARGET_PROC tenha falhado
        return -ENOMEM;
    }

    printk(KERN_INFO "Syscall Counter Module carregado\n");
    return 0;
}

/**
 * Função que é chamada ao descarregar o módulo, removendo o kprobe e removendo os arquivos TARGET_PROC e COUNTER_PROC
 */
static void __exit syscall_counter_exit(void)
{
    // Remove o kprobe
    unregister_kprobe(&kp);

    // Remove o COUNTER_PROC file se existir
    if (counter_file)
    {
        printk(KERN_INFO "Removendo counter file");
        proc_remove(counter_file); // Remove o arquivo COUNTER_PROC
    }

    // Remove o TARGET_PROC file se existir
    if (target_file)
    {
        printk(KERN_INFO "Removendo target file");
        proc_remove(target_file); // Remove o arquivo TARGET_PROC
    }
    printk(KERN_INFO "Syscall Counter Module descarregado\n");
}

module_init(syscall_counter_init); // Define a função a ser chamada ao carregar o módulo
module_exit(syscall_counter_exit); // Define a função a ser chamada ao descarregar o módulo

// Labels de versão do módulo
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Módulo de Contador de Syscalls usando kprobes");
MODULE_VERSION("0.1");
