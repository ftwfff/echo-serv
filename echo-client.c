#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <net/sock.h>

#define PORT 2325

struct service {
	struct socket *socket;
	struct task_struct *thread;
};

struct service *svc;

int recv_msg(struct socket *sock, unsigned char *buf, int len) 
{
	struct msghdr msg;
	struct kvec iov;
	int size = 0;

	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;
	msg.msg_name = 0;
	msg.msg_namelen = 0;

	size = kernel_recvmsg(sock, &msg, &iov, 1, len, msg.msg_flags);

	printk(KERN_ALERT "the message is : %s\n",buf);

	return size;
}

int send_msg(struct socket *sock,char *buf,int len) 
{
	struct msghdr msg;
	struct kvec iov;
	int size;

	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;
	msg.msg_name = 0;
	msg.msg_namelen = 0;

	size = kernel_sendmsg(sock, &msg, &iov, 1, len);

	printk(KERN_INFO "message sent!\n");

	return size;
}

int start_sending(void)
{
	int error, i, size, ip;
	struct sockaddr_in sin;
	int len = 15;
	unsigned char buf[len+1];
	const char text[] = "hello world!\n";

	ip = (10 << 24) | (1 << 16) | (1 << 8) | (3);
	sin.sin_addr.s_addr = htonl(ip);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);

	i = 0;
	while (!kthread_should_stop()) {
		error = sock_create_kern(PF_INET,SOCK_STREAM,IPPROTO_TCP,&svc->socket);
		if(error<0) {
			printk(KERN_ERR "cannot create socket\n");
			goto wait;
		}

		error = kernel_connect(svc->socket, (struct sockaddr*)&sin, sizeof(struct sockaddr_in), 0);
		if(error<0) {
			printk(KERN_ERR "cannot connect socket\n");
			goto wait;
		}
		printk(KERN_ERR "sock %d connected\n", i++);

		strncpy((char*)&buf, &text[0], 14);
		send_msg(svc->socket, buf, 13);
		size = recv_msg(svc->socket, buf, len);

		kernel_sock_shutdown(svc->socket, SHUT_RDWR);
		msleep(1000);
	}
	sock_release(svc->socket);
wait:
	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		schedule();
		set_current_state(TASK_INTERRUPTIBLE);
	}
	__set_current_state(TASK_RUNNING);

	return 0;
}

static int __init mod_init(void)
{
	svc = kmalloc(sizeof(struct service), GFP_KERNEL);
	svc->thread = kthread_run((void *)start_sending, NULL, "echo-client");
	printk(KERN_ALERT "echo-client module loaded\n");

        return 0;
}

static void __exit mod_exit(void)
{
	//if (svc->socket != NULL) {
	//	kernel_sock_shutdown(svc->socket, SHUT_RDWR);
	//	sock_release(svc->socket);
	//	printk(KERN_ALERT "release socket\n");
	//}
	
	kthread_stop(svc->thread);

	kfree(svc);
	printk(KERN_ALERT "removed client-serv module\n");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("TCP/UDP echo client in the kernel");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Douwe De Bock <douwe.debock@ugent.be>");
