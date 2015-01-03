#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/spinlock.h>

#include <net/sock.h>

#define PORT 2325

struct service {
	struct socket *socket;
	struct task_struct *thread;
	int running;
};

DEFINE_SPINLOCK(lock);
struct service *svc;

int recv_msg(struct socket *sock, struct sockaddr_in *cl,
		unsigned char *buf, int len) 
{
	struct msghdr msg;
	struct kvec iov;
	int size = 0;

	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;
	msg.msg_name = cl;
	msg.msg_namelen = sizeof(struct sockaddr_in);

	size = kernel_recvmsg(sock, &msg, &iov, 1, len, msg.msg_flags);

	if (size > 0)
		printk(KERN_ALERT "the message is : %s\n",buf);

	return size;
}

int send_msg(struct socket *sock, struct sockaddr_in *cl, char *buf, int len) 
{
	struct msghdr msg;
	struct kvec iov;
	int size;

	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;
	msg.msg_name = cl;
	msg.msg_namelen = sizeof(struct sockaddr_in);

	size = kernel_sendmsg(sock, &msg, &iov, 1, len);

	if (size > 0)
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
		error = sock_create_kern(PF_INET, SOCK_DGRAM, IPPROTO_UDP,
				&svc->socket);
		if(error<0) {
			printk(KERN_ERR "cannot create socket\n");
			spin_lock(lock);
			svc->running = 0;
			spin_unlock(lock);
			return -1;
		}

		strncpy((char*)&buf, &text[0], 14);
		send_msg(svc->socket, &sin, buf, 13);
		size = recv_msg(svc->socket, &sin, buf, len);

		msleep(1000);
	}
	sock_release(svc->socket);

	spin_lock(lock);
	svc->running = 0;
	spin_unlock(lock);

	return 0;
}

static int __init mod_init(void)
{
	svc = kmalloc(sizeof(struct service), GFP_KERNEL);
	
	spin_lock(lock);
	svc->running = 1;
	spin_unlock(lock);

	svc->thread = kthread_run((void *)start_sending, NULL, "echo-client");
	printk(KERN_ALERT "echo-client module loaded\n");

        return 0;
}

static void __exit mod_exit(void)
{
	spin_lock(lock);
	if (svc->running) {
		spin_unlock(lock);
		kthread_stop(svc->thread);
	} else {
		spin_unlock(lock);
	}

	kfree(svc);
	printk(KERN_ALERT "removed client-serv module\n");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("TCP/UDP echo client in the kernel");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Douwe De Bock <douwe.debock@ugent.be>");
