# Nome do módulo
obj-m := syscall_counter.o

# Diretório de cabeçalhos do kernel
KDIR := /lib/modules/$(shell uname -r)/build

# Diretório atual
PWD := $(shell pwd)

# Alvo de compilação do módulo
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Alvo para limpar os arquivos gerados pela compilação
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
