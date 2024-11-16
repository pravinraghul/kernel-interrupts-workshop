obj-m += btnintp.o
USRAPP=app

all: build

build: module app

module:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

app:
	$(CC) $(USRAPP).c -o $(USRAPP)

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm app -rf

load:
	sudo insmod btnintp.ko

unload:
	sudo rmmod btnintp

dist-clean:
	rm *~ -rf