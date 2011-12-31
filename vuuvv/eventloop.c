#include "vuuvv.h"

v_inline(void)
v_close_listening(v_listening_t *ls)
{
	if (v_close_socket((ls->event->fd)) == INVALID_SOCKET) {
		v_log_error(V_LOG_ERROR, v_socket_errno, v_close_socket_name " failed: ");
	}
	v_free(ls->event);
	v_free(ls->sockaddr);
	v_free(ls->addr_text);
	v_free(ls->buffer);
	if (ls->connection != NULL)
		v_close_connection(ls->connection);
	v_free(ls);
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
		ev[i].type = V_IO_NONE;
		ev[i].status = V_IO_STATUS_CLOSED;
		ev[i].fd = (v_socket_t) -1;
		ev[i].data = c;
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
	struct sockaddr_in      *host_addr;
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

	host_addr = v_malloc(sizeof(struct sockaddr_in));
	if (host_addr == NULL) {
		v_log_error(V_LOG_ALERT, v_errno, "v_malloc failed: ");
		v_free(ls);
		v_free(ev);
		return NULL;
	}

	v_memzero(ls, sizeof(v_listening_t));
	v_memzero(ev, sizeof(v_io_event_t));

	if (v_io_prepare(ev) == V_ERR) {
		v_free(ls);
		v_free(ev);
		v_free(host_addr);
		return NULL;
	}

	ls->event = ev;
	ev->data = ls;

	host_addr->sin_family = AF_INET;
	host_addr->sin_port = htons(port);
	if (hostname == NULL) {
		host_addr->sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		if ((host_addr->sin_addr.s_addr = inet_addr(hostname)) == INADDR_NONE) {
			v_log(V_LOG_ALERT, "Host address invalid %s", hostname);
			v_close_listening(ls);
			return NULL;
		}
	}

	if (bind(ev->fd, (SOCKADDR *)host_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
		v_log_error(V_LOG_ALERT, v_socket_errno, "bind() failed: ");
		v_close_listening(ls);
		return NULL;
	}

	backlog = (backlog == 0) ? SOMAXCONN : backlog;
	if (listen(ev->fd, backlog) == SOCKET_ERROR) {
		v_log_error(V_LOG_ALERT, v_socket_errno, "listen() failed: ");
		v_close_listening(ls);
		return NULL;
	}

	ls->sockaddr = host_addr;
	ev->type = V_IO_NONE;
	ev->status = V_IO_STATUS_LISTENING;

	return ls;
}

int
v_connect(const char *hostname, int port, v_io_proc handler)
{
	v_connection_t      *c;
	struct sockaddr_in  *remote_addr;

	remote_addr = v_malloc(sizeof(struct sockaddr_in));
	remote_addr->sin_family = AF_INET;
	remote_addr->sin_port = htons(port);
	if ((remote_addr->sin_addr.s_addr = inet_addr(hostname)) == INADDR_NONE) {
		v_log(V_LOG_ALERT, "Host address invalid %s", hostname);
		v_free(remote_addr);
		return V_ERR;
	}

	c = v_get_connection();
	c->remote_addr = remote_addr;

	return v_io_add(c->event, V_IO_CONNECT, handler);
}

/**
 * Get a connection and prepare it.
 * Parameters:
 *      None
 * Returns:
 *      If success return a connection with V_IO_STATUS_READY status, 
 *      else NULL.
 * Throws:
 *      None.
 **/
v_connection_t *
v_get_connection()
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

	v_io_prepare(c->event);
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
	c->local_addr = NULL;
	c->remote_addr = NULL;
	c->event->type = V_IO_NONE;
}

void
v_close_connection(v_connection_t *c)
{
	v_io_event_t    *ev = c->event;

	switch(ev->status) {
		case V_IO_STATUS_STANDBY_C:
		case V_IO_STATUS_ESTABLISHED:
			v_io_add(ev, V_IO_CLOSE, NULL);
			return;
		case V_IO_STATUS_CLOSING:
			v_log(V_LOG_ALERT, "Close a connection in 'CLOSING' status");
			return;
		case V_IO_STATUS_CLOSED:
		case V_IO_STATUS_USABLE:
		case V_IO_STATUS_READY:
		case V_IO_STATUS_CONNECTING:
		case V_IO_STATUS_STANDBY_L:
			if (v_close_socket(ev->fd) == -1) {
				v_log_error(V_LOG_ERROR, v_socket_errno, v_close_socket_name " failed: ");
			}
			ev->fd = (v_socket_t) -1;
			ev->status = V_IO_STATUS_CLOSED;
			v_free_connection(c);
			return;
		default:
			v_log(V_LOG_ALERT, "Close a connection in Unknown status: %d", ev->status);
			return;
	}
}
