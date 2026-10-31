#include "../include/add.h"

#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>



int stop = false;
int sockfd;

struct int_vec {
	pthread_mutex_t m;
	int *data;
	int len;
	int cap;
};

void vec_init(struct int_vec *vec)
{
	pthread_mutex_init(&vec->m, NULL);
	vec->len = vec->cap = 0;
	vec->data = NULL;
}

void vec_push(struct int_vec *vec, int e)
{
	pthread_mutex_lock(&vec->m);
	if (vec->cap == 0) {
		vec->cap = 16;
		vec->data = malloc(sizeof(int) * vec->cap);
	}
	if (vec->cap == vec->len) {
		vec->cap *= 2;
		vec->data = realloc(vec->data, sizeof(int) * vec->cap);
	}

	vec->data[vec->len++] = e;
	pthread_mutex_unlock(&vec->m);
}

void vec_remove(struct int_vec *vec, int e)
{
	pthread_mutex_lock(&vec->m);
	if (vec->len == 1) {
		vec->len--;
	} else {
		for (int i = 0; i < vec->len; i++) {
			if (vec->data[i] == e) {
				if (i == vec->len - 1)
					vec->len--;
				else
					vec->data[i] = vec->data[--vec->len];
				break;
			}
		}
	}
	pthread_mutex_unlock(&vec->m);
}

void stop_app(int sig){
	stop = true;
	close(sockfd);
}

void thread_close(pthread_t *thread_IDs, int thread_number){
	for(int i = 0; i < thread_number; i++){
		pthread_join(thread_IDs[i], NULL);
	}
}

struct handler_arg {
	int thread_fd;
	struct int_vec *vec;
};

void *handler_task(void *arg_)
{
	struct handler_arg arg = *(struct handler_arg *) arg_;
	int thread_fd = arg.thread_fd;
	struct int_vec *vec = arg.vec;
	free(arg_);

	char buf[128];
	int readbytes;

	vec_push(vec, thread_fd);
	while (stop == false) {
		readbytes = read(thread_fd, &buf, sizeof(buf));

		pthread_mutex_lock(&vec->m);
		for (int i = 0; i < vec->len; i++) {
			if (vec->data[i] != thread_fd)
				write(vec->data[i], buf, readbytes);
		}
		pthread_mutex_unlock(&vec->m);

		printf("%d\n", thread_fd);
		if (dprintf(thread_fd, "you're %d\n", thread_fd) < 0)
			break;
	}

	vec_remove(vec, thread_fd);
	close(thread_fd);

	return NULL;
}

int main(void)
{
	printf("2 + 3 = %d\n", add(2, 3));

	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, stop_app);

	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	struct sockaddr_in addr = {
		.sin_port = htons(3000),
		.sin_addr = 0,
	};

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int) { 1 }, sizeof(int));

	assert(bind(sockfd, (const struct sockaddr *) &addr, sizeof(addr)) == 0);

	listen(sockfd, 8);

	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);

	pthread_t* thread_IDs = malloc(sizeof(pthread_t));
	int thread_number = 0;

	struct int_vec thread_fds;
	vec_init(&thread_fds);

	while (stop == false) {
		struct sockaddr_in remote_addr;
		socklen_t socklen = sizeof(remote_addr);

		int remote_fd = accept(sockfd, (struct sockaddr *) &remote_addr, &socklen);

		struct handler_arg *arg = malloc(sizeof(struct handler_arg));
		arg->thread_fd = remote_fd;
		arg->vec = &thread_fds;

		pthread_t thread;
		pthread_create(&thread,
				&thread_attr,
				handler_task,
				arg);

		thread_number++;
		thread_IDs = realloc(thread_IDs, sizeof(pthread_t) * (thread_number));
		thread_IDs[thread_number - 1] = thread;

	}
	// global_durdurma_sinyali = true;

	// pthread_close;
	// TODO: bütün thread'lere join ol
	thread_close(thread_IDs, thread_number);

	return 0;
}
