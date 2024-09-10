# Projeto 02 da Disciplina de SO sobre módulos do kernel linux

Para esse projeto foi implementado um módulo simples para o kernel linux, que consiste em um contador de `syscalls` usando a biblioteca `kprobes`.  
Sempre que o `kprobes` detecta que a `syscall` informada no arquivo `TARGET_PROC` foi chamada, ele incremente um contador que pode ter seu valor acessado via `cat /proc/COUNTER_PROC`.
É importante ressaltar que o kprobes não monitora especificamente syscalls, mas sim funções do kernel.

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
## "Syscall Counter Module carregad"

# Para descarregar o módulo
rmmod syscall_counter

# Verifique que o módulo foi descarregado com mensagem:
## "Syscall Counter Module descarregado"
```

## Teste
Por padrão a syscall monitorada é a `sys_newuname`, então sempre que `uname -a` for chamado, o contador será incrementado.
```bash
# Monitore os logs pelo dmesg
sudo dmesg -wH # wH implica num modo watch

# Chame o comando
uname -a

# Verifique que o contador aumentou
cat /proc/COUNTER_PROC
```

A syscall monitorada pode ser controlada escrevendo para o arquivo `TARGET_PROC`. O `symbol` que o `kprobe` aceita, geralmente, vêm do arquivo `/proc/kallsyms`:
```bash
cat /proc/kallsyms | grep "<syscall>"

# Exemplo "sys_open"
## Escolher __x64_sys_open, geralmente é o equivalente a syscall em arquiteturas x64

# Escrever para o TARGET_PROC
sudo echo -n "<kallsyms_symbol>" > /proc/TARGET_PROC # o -n remove caracteres de quebra de linha

# Se tudo ocorrer bem, o dmesg deve exibir a mensagem:
# kprobe registrado para syscall: <kallsyms_symbol>
```
### Lista de syscalls interessantes
- `sys_open`: `__x64_sys_open`, geralmente não é chamada, boa para testar o zeramento do contador.
- `sys_newuname`: `__x64_sys_newuname`, não gera tantas chamadas, boa para monitorar o funcionamento.
- `sys_read`: `__x64_sys_read`, gera muitas chamadas.
