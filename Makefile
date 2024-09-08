# Nome do módulo
obj-m := syscall_counter.o

# Diretório de cabeçalhos do kernel
## Obtém a versão do kernel em execução e a utiliza para 
## especificar o diretório de compilação do novo módulo
KDIR := /lib/modules/$(shell uname -r)/build

# Diretório atual
PWD := $(shell pwd)

# Compilar o módulo
## KDIR: Muda para o diretório de compilação do kernel
## M=(PWD): Seta o diretório de compilação para o atual
## modules: regra para compilar o kernel
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Limpar os arquivos gerados pela compilação
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
