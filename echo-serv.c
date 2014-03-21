#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/slab.h>

#include <net/sock.h>

#define PORT 2325

struct service {
	struct socket *listen_socket;
	struct task_struct *thread;
};

struct service *svc;

int start_listen(void)
{
	struct socket *acsock;
	int error;
	struct sockaddr_in sin;

	error = sock_create(PF_INET,SOCK_STREAM,IPPROTO_TCP,&svc->listen_socket);
	if(error<0) {
		printk(KERN_ERR "cannot create socket");
		return -1;
	}

	sin.sin_addr.s_addr=htonl(INADDR_ANY);
	sin.sin_family=AF_INET;
	sin.sin_port=htons(PORT);

	error = svc->listen_socket->ops->bind(svc->listen_socket,(struct sockaddr*)&sin,sizeof(sin));
	if(error<0) {
		printk(KERN_ERR "cannot bind socket");
		return -1;
	}

	error = svc->listen_socket->ops->listen(svc->listen_socket,5);
	if(error<0) {
		printk(KERN_ERR "cannot listen to socket");
		return -1;
	}

	acsock = kmalloc(sizeof(struct socket), GFP_KERNEL);
	while (!kthread_should_stop()) {
		error = sock_create(PF_INET,SOCK_STREAM,IPPROTO_TCP,&acsock);
		if(error<0) {
			printk(KERN_ERR "cannot create accept socket");
			kfree(acsock);
			return -1;
		}
		error = svc->listen_socket->ops->accept(svc->listen_socket, acsock, 0);
		if(error<0) {
			printk(KERN_ERR "cannot accept socket");
			sock_release(acsock);
			kfree(acsock);
			return -1;
		}
		printk(KERN_ERR "sock accepted");
		sock_release(acsock);
	}
	kfree(acsock);
	return 0;
}

static int __init mod_init(void)
{
	svc = kmalloc(sizeof(struct service), GFP_KERNEL);
	svc->thread = kthread_run((void *)start_listen, NULL, "ktcp");
	printk(KERN_ALERT "echo-serv module loaded");

        return 0;
}

static void __exit mod_exit(void)
{
	kthread_stop(svc->thread);
	if (svc->listen_socket != NULL) {
		sock_release(svc->listen_socket);
	}

	kfree(svc);
	printk(KERN_ALERT "removed echo-serv module");
	
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("TCP/UDP echo server in the kernel");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Douwe De Bock <douwe.debock@ugent.be>");
