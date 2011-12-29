#include "vuuvv.h"

v_inline(void)
v_shutdown_socket(fd)
{
	if (v_close_socket((fd)) == INVALID_SOCKET) {
		v_log_error(V_LOG_ERROR, v_socket_errno, v_close_socket_name " failed: ");
	}      
}

int
v_eventloop_init()
{
	size_t          i, connections_count;
	v_io_event_t    *ev;
	v_connection_t  *c, *next;

	connections_count = v_config.connections_count;
	c = v_new(v_connection_t, connections_count);
	if (c == NULL) {
		v_log_error(V_LOG_ERROR, v_errno, "Alloc memory failed: ");
		return V_ERR;
	}

	ev = v_new(v_io_event_t, connections_count);
	if (ev == NULL) {
		v_log_error(V_LOG_ERROR, v_errno, "Alloc memory failed: ");
		return V_ERR;
	}

	for (i = 0; i < connections_count; i++) {
		ev[i].type = V_IO_ERROR;
		ev[i].ready = 0;
		ev[i].closed = 1;
		ev[i].fd = (v_socket_t) -1;
	}

	i = connections_count;
	next = NULL;
	do {
		i--;

		c[i].data = next;
		c[i].event = &ev[i];

		next = &c[i];

	} while(i);

	v_config.free_connections = next;
	v_config.free_connections_count = connections_count;
	v_config.connections = c;

	return V_OK;
}

v_listening_t *
v_create_listening(const char *hostname, int port, int backlog)
{
	struct sockaddr_in      host_addr;
	v_socket_t              fd;
	v_listening_t           *ls;
	v_io_event_t            *ev;

	ls = v_malloc(sizeof(v_listening_t));
	if (ls == NULL) {
		v_log_error(V_LOG_ALERT, v_errno, "v_malloc failed: ");
		return NULL;
	}

	ev = v_malloc(sizeof(v_io_event_t));
	if (ev == NULL) {
		v_log_error(V_LOG_ALERT, v_errno, "v_malloc failed: ");
		v_free(ls);
		return NULL;
	}

	v_memzero(ls, sizeof(v_listening_t));
	v_memzero(ev, sizeof(v_io_event_t));

	backlog = (backlog == 0) ? SOMAXCONN : backlog;
	fd = v_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == INVALID_SOCKET) {
		v_log_error(V_LOG_ALERT, v_socket_errno, v_socket_name " failed: ");
		v_free(ls);
		v_free(ev);
		return NULL;
	}

	ls->event = ev;
	ev->fd = fd;
	ev->data = ls;

	host_addr.sin_family = AF_INET;
	host_addr.sin_port = htons(port);
	if (hostname == NULL) {
		host_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		if ((host_addr.sin_addr.s_addr = inet_addr(hostname)) == INADDR_NONE) {
			v_log(V_LOG_ALERT, "Host address invalid %s", hostname);
			v_shutdown_socket(fd);
			v_free(ls);
			v_free(ev);
			return NULL;
		}
	}

	if (bind(fd, (SOCKADDR *)&host_addr, sizeof(host_addr)) == SOCKET_ERROR) {
		v_log_error(V_LOG_ALERT, v_socket_errno, "bind() failed: ");
		v_shutdown_socket(fd);
		v_free(ls);
		v_free(ev);
		return NULL;
	}

	if (listen(fd, backlog) == SOCKET_ERROR) {
		v_log_error(V_LOG_ALERT, v_socket_errno, "listen() failed: ");
		v_shutdown_socket(fd);
		v_free(ls);
		v_free(ev);
		return NULL;
	}

	ev->type = V_IO_ERROR;
	ev->ready = 0;
	ev->closed = 1;

	return ls;
}

v_connection_t *
v_get_connection(v_socket_t fd)
{
	v_connection_t      *c;

	c = v_config.free_connections;

	if (c == NULL) {
		/* TODO: create new connections for free_connections */
		v_log(V_LOG_ERROR, "%ui worker_connections are not enough", v_config.connections_count);
		return NULL;
	}

	v_config.free_connections = c->data;
	v_config.free_connections_count--;

	c->event->fd = fd;

	c->event->data = c;

	return c;
}

void
v_free_connection(v_connection_t *c)
{
	c->data = v_config.free_connections;
	v_config.free_connections = c;
	v_config.free_connections_count++;
	v_free(c->local_addr);
	v_free(c->remote_addr);
}
