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

int recv_msg(struct socket *sock, struct sockaddr_in *cl, unsigned char *buf, int len) 
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

int start_listen(void)
{
	int error, i, size;
	struct sockaddr_in sin, client;
	int len = 15;
	unsigned char buf[len+1];

	error = sock_create_kern(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &svc->listen_socket);
	if(error<0) {
		printk(KERN_ERR "cannot create socket\n");
		return -1;
	}

	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);

	error = kernel_bind(svc->listen_socket, (struct sockaddr*)&sin, sizeof(sin));
	if(error < 0) {
		printk(KERN_ERR "cannot bind socket, error code: %d\n", error);
		return -1;
	}

	i = 0;
	while (1) {
		memset(&buf, 0, len+1);
		memset(&client, 0, sizeof(struct sockaddr_in));
		size = recv_msg(svc->listen_socket, &client, buf, len);
		if (size < 0) {
			return -1;
		}
		send_msg(svc->listen_socket, &client, buf, size);
	}

	return 0;
}

static int __init mod_init(void)
{
	svc = kmalloc(sizeof(struct service), GFP_KERNEL);
	svc->thread = kthread_run((void *)start_listen, NULL, "echo-serv");
	printk(KERN_ALERT "echo-serv module loaded\n");

        return 0;
}

static void __exit mod_exit(void)
{
	if (svc->listen_socket != NULL) {
		svc->listen_socket->ops->release(svc->listen_socket);
		printk(KERN_ALERT "release socket\n");
	}
	
	kfree(svc);
	printk(KERN_ALERT "removed echo-serv module\n");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("TCP/UDP echo server in the kernel");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Douwe De Bock <douwe.debock@ugent.be>");
