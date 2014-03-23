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

int recv_msg(struct socket *sock, unsigned char *buf, int len) 
{
	struct msghdr msg;
	struct kvec iov;
	int size = 0;

	printk(KERN_INFO "recv_msg\n");

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

	printk(KERN_INFO "send msg\n");

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

int start_listen(void)
{
	struct socket *acsock;
	int error, i, size;
	struct sockaddr_in sin;
	int len = 15;
	unsigned char buf[len+1];

	error = sock_create_kern(PF_INET,SOCK_STREAM,IPPROTO_TCP,&svc->listen_socket);
	if(error<0) {
		printk(KERN_ERR "cannot create socket\n");
		goto wait;
	}

	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);

	error = kernel_bind(svc->listen_socket,(struct sockaddr*)&sin,sizeof(sin));
	if(error < 0) {
		printk(KERN_ERR "cannot bind socket, error code: %d\n", error);
		goto wait;
	}

	error = kernel_listen(svc->listen_socket,5);
	if(error<0) {
		printk(KERN_ERR "cannot listen to socket, error code: %d\n", error);
		goto wait;
	}

	acsock = kmalloc(sizeof(struct socket), GFP_KERNEL);
	i = 0;
	while (!kthread_should_stop()) {
		error = kernel_accept(svc->listen_socket, &acsock, 0);
		if(error<0) {
			printk(KERN_ERR "cannot accept socket\n");
			kfree(acsock);
			goto wait;
		}
		printk(KERN_ERR "sock %d accepted\n", i++);

		memset(&buf, 0, len+1);
		size = recv_msg(acsock, buf, len);
		send_msg(acsock, buf, size);

		kernel_sock_shutdown(acsock, SHUT_RDWR);
		sock_release(acsock);
	}
	kfree(acsock);
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
	svc->thread = kthread_run((void *)start_listen, NULL, "echo-serv");
	printk(KERN_ALERT "echo-serv module loaded\n");

        return 0;
}

static void __exit mod_exit(void)
{
	if (svc->listen_socket != NULL) {
		kernel_sock_shutdown(svc->listen_socket, SHUT_RDWR);
		svc->listen_socket->ops->release(svc->listen_socket);
		printk(KERN_ALERT "release socket\n");
	}
	
	kthread_stop(svc->thread);

	kfree(svc);
	printk(KERN_ALERT "removed echo-serv module\n");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("TCP/UDP echo server in the kernel");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Douwe De Bock <douwe.debock@ugent.be>");
