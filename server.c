#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include "./lib/hash_table.h"

#define UNUSED(x)	((void)(x))
#define BUF_MAXLEN	1024
#define LOG_ERR_OPT	(LOG_SYSLOG | LOG_ERR)
#define LOG_INFO_OPT	(LOG_SYSLOG | LOG_INFO)

typedef struct thread_info {
	pthread_t thread_id;
	int pfd;
	int afd;
	hash_table_t *storage;
} thread_info_t;

static thread_info_t thread_info = {
	.thread_id = 0,
	.pfd = 0,
	.afd = 0,
	.storage = NULL
};

static void signal_handler_callback(int signum)
{
	hash_table_destroy(thread_info.storage);
	close(thread_info.pfd);
	_exit(0);
}

static void register_signal_handler(void)
{
	struct sigaction sa;

	sa.sa_flags = SA_RESTART;
	sa.sa_handler = signal_handler_callback;

	sigemptyset(&sa.sa_mask);

	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGKILL, &sa, NULL);
}

static void *process(void *arg)
{
	thread_info_t *tinfo = (thread_info_t *)(arg);
	char buf[BUF_MAXLEN], response[BUF_MAXLEN];

	for (;;) {
		bzero(buf, BUF_MAXLEN);

		if (recv(tinfo->afd, buf, BUF_MAXLEN, 0) <= 0) {
			shutdown(tinfo->afd, SHUT_RDWR);
			close(tinfo->afd);
			pthread_exit(NULL);
			break;
		}

		buf[strlen(buf) - 1] = '\0';

		char *token = strtok(buf, " ");
		char *errmsg;

		// process 'GET' command.
		if (token != NULL && !strcmp(token, "GET")) {
			char *key = strtok(NULL, " ");

			if (key == NULL) {
				errmsg = strdup("-ERR wrong number of arguments for 'GET' command.\n");
				syslog(LOG_ERR_OPT, "%s", errmsg);
				send(tinfo->afd, errmsg, strlen(errmsg), 0);
				free(errmsg);
				continue;
			}

			hash_table_t *ht = hash_table_search(tinfo->storage, key);

			if (ht == NULL) {
				errmsg = strdup("$-1\n");
				syslog(LOG_ERR_OPT, "Data with key '%s' not found.\n", key);
				send(tinfo->afd, errmsg, strlen(errmsg), 0);
				free(errmsg);
				continue;
			}

			bzero(response, BUF_MAXLEN);
			sprintf(response, "$%ld: %s\n", strlen(ht->value), ht->value);
			send(tinfo->afd, response, strlen(response), 0);
			continue;
		}

		// process 'SET' command.
		if (token != NULL && !strcmp(token, "SET")) {
			char *key = strtok(NULL, " ");
			char *value = strtok(NULL, " ");

			if (key == NULL || value == NULL) {
				errmsg = strdup("-ERR wrong number of arguments for 'SET' command.\n");
				syslog(LOG_ERR_OPT, "%s", errmsg);
				send(tinfo->afd, errmsg, strlen(errmsg), 0);
				free(errmsg);
				continue;
			}

			int ret = hash_table_insert(tinfo->storage, key, value);

			if (ret < 0) {
				bzero(response, BUF_MAXLEN);
				sprintf(response, "-ERR failed to set data with key '%s'.\n", key);
				syslog(LOG_ERR_OPT, "%s", response);
				send(tinfo->afd, response, strlen(response), 0);
				continue;
			}

			bzero(response, BUF_MAXLEN);
			sprintf(response, "+OK\n");
			send(tinfo->afd, response, strlen(response), 0);
			continue;
		}

		// process 'DELETE' command.
		if (token != NULL && !strcmp(token, "DELETE")) {
			char *key = strtok(NULL, " ");

			if (key == NULL) {
				errmsg = strdup("-ERR wrong number of arguments for 'DELETE' command.\n");
				syslog(LOG_ERR_OPT, "%s", errmsg);
				send(tinfo->afd, errmsg, strlen(errmsg), 0);
				continue;
			}

			int ret = hash_table_delete(tinfo->storage, key);

			if (ret < 0) {
				bzero(response, BUF_MAXLEN);
				sprintf(response, "-ERR failed to delete data with key '%s'.\n", key);
				syslog(LOG_ERR_OPT, "%s", response);
				send(tinfo->afd, response, strlen(response), 0);
				continue;
			}

			bzero(response, BUF_MAXLEN);
			sprintf(response, "+OK\n");
			send(tinfo->afd, response, strlen(response), 0);
			continue;
		}

		bzero(response, BUF_MAXLEN);
		sprintf(response, "-ERR unknown command '%s'\n", token);
		send(tinfo->afd, response, strlen(response), 0);
		continue;
	}
}

static void do_loop(int fd)
{
	int afd, ret;
	pthread_attr_t attr;

	for (;;) {
		if (listen(fd, 0) < 0) {
			syslog(LOG_ERR_OPT, "listen() failed.\n");
			break;
		}

		if ((afd = accept(fd, NULL, NULL)) < 0) {
			syslog(LOG_ERR_OPT, "accept() failed.\n");
			break;
		}

		thread_info.afd = afd;

		if ((ret = pthread_attr_init(&attr)) != 0) {
			syslog(LOG_ERR_OPT, "pthread_attr_init() failed (errno: %d).\n", ret);
			break;
		}

		if ((ret = pthread_create(&thread_info.thread_id, &attr, &process, &thread_info)) != 0) {
			syslog(LOG_ERR_OPT, "pthread_create() failed (errno: %d)\n", ret);
			break;
		}

		if ((ret = pthread_attr_destroy(&attr)) != 0) {
			syslog(LOG_ERR_OPT, "pthread_attr_destroy() failed (errno: %d)\n", ret);
			break;
		}

		if ((ret = pthread_detach(thread_info.thread_id)) != 0) {
			syslog(LOG_ERR_OPT, "pthread_detach() failed (errno: %d)\n", ret);
			break;
		}
	}
}

int main(int argc, char **argv)
{
	UNUSED(argc);
	UNUSED(argv);

	register_signal_handler();

	int sock_opt = 1;
	struct sockaddr_in sa;

	if ((thread_info.pfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		syslog(LOG_SYSLOG | LOG_ERR, "socket() failed.\n");
		return 1;
	}

	if (setsockopt(thread_info.pfd, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt)) < 0) {
		syslog(LOG_ERR_OPT, "setsockopt() failed (context: SOL_SOCKET, option: SO_REUSEADDR, value = 1)\n");
		return 1;
	}

	if (setsockopt(thread_info.pfd, SOL_SOCKET, SO_KEEPALIVE, &sock_opt, sizeof(sock_opt)) < 0) {
		syslog(LOG_ERR_OPT, "setsockopt() failed (context: SOL_SOCKET, option: SO_KEEPALIVE, value = 1)\n");
		return 1;
	}

	if (setsockopt(thread_info.pfd, IPPROTO_TCP, TCP_NODELAY, &sock_opt, sizeof(sock_opt)) < 0) {
		syslog(LOG_ERR_OPT, "setsockopt() failed (context: IPPROTO_TCP, option: TCP_NODELAY, value = 1)\n");
		return 1;
	}

	bzero(&sa, sizeof(struct sockaddr_in));

	sa.sin_family = AF_INET;
	sa.sin_port = htons(1337);
	sa.sin_addr.s_addr = INADDR_ANY;

	if (bind(thread_info.pfd, (struct sockaddr *)&sa, (socklen_t)sizeof(struct sockaddr)) < 0) {
		syslog(LOG_ERR_OPT, "bind() failed.\n");
		return 1;
	}

	thread_info.storage = hash_table_create();

	if (thread_info.storage == NULL) {
		syslog(LOG_ERR_OPT, "Cannot allocate memory for hash-table based storage.\n");
		return 1;
	}

	do_loop(thread_info.pfd);

	return 0;
}
