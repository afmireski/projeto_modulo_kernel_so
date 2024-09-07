# Projeto 02 da Disciplina de SO sobre módulos do kernel linux

Para esse projeto foi implementado um módulo simples para o kernel linux, que consiste em um contador de `syscalls` usando a biblioteca `kprobes`.  
Sempre que o `kprobes` detecta que a `syscall` informada em `TARGET_SYSCALL` foi chamada, ele incremente um contador que pode ter seu valor acessado via `cat /proc/syscall_counter`.

## Setup
```bash
# Clone o projeto
git clone https://github.com/afmireski/projeto_modulo_kernel_so.git

# Entre na pasta do projeto
cd /projeto_modulo_kernel_so

# Compile o módulo
make

## Se precisar limpar os arquivos
make clean

# Abra um terminal a parte e acesse os logs do sistema
sudo dmesg -wH # wH implica num modo watch

# Carregue o módulo do kernel
sudo insmod syscall_counter.ko

# Verifique que o módulo foi carregado com a mensagem:
## "Syscall Counter Module Carregado"

# Para descarregar o módulo
rmmod syscall_counter

# Verifique que o módulo foi descarregado com mensagem:
## "Syscall Counter Module Descarregado"
```

## Teste
Suponha que a syscall monitorada seja a `sys_newuname`, então sempre que `uname -a` for chamado, o contador será incrementado.
```bash
# Monitore os logs pelo dmesg
sudo dmesg -wH # wH implica num modo watch

# Chame o comando
uname -a

# Verifique que o contador aumentou
cat /proc/syscall_counter
```

