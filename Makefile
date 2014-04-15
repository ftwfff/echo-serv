obj-m += echo-serv-tcp.o
obj-m += echo-client-tcp.o
obj-m += echo-serv-udp.o
obj-m += echo-client-udp.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
