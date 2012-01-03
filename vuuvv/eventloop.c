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
	v_free(ls->head);
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
	c = v_new_n(v_connection_t, connections_count);
	if (c == NULL) {
		v_log_error(V_LOG_ERROR, v_errno, "Alloc memory failed: ");
		return V_ERR;
	}

	ev = v_new_n(v_io_event_t, connections_count);
	if (ev == NULL) {
		v_log_error(V_LOG_ERROR, v_errno, "Alloc memory failed: ");
		v_free(c);
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

		c[i].next = next;
		c[i].event = &ev[i];
		c[i].head = NULL;

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
	char                    *buf;
	v_connection_t          **head;

	ls = v_new(v_listening_t);
	if (ls == NULL) {
		v_log_error(V_LOG_ALERT, v_errno, "v_malloc failed: ");
		return NULL;
	}

	buf = v_malloc(2 * sizeof(struct sockaddr) + 16);
	if (buf == NULL) {
		v_log_error(V_LOG_ALERT, v_errno, "v_malloc failed: ");
		v_free(ls);
		return NULL;
	}

	ev = v_new(v_io_event_t);
	if (ev == NULL) {
		v_log_error(V_LOG_ALERT, v_errno, "v_malloc failed: ");
		v_free(ls);
		v_free(buf);
		return NULL;
	}

	host_addr = v_new(struct sockaddr_in);
	if (host_addr == NULL) {
		v_log_error(V_LOG_ALERT, v_errno, "v_malloc failed: ");
		v_free(ls);
		v_free(buf);
		v_free(ev);
		return NULL;
	}

	head = v_new(void *);
	if (head == NULL) {
		v_log_error(V_LOG_ALERT, v_errno, "v_malloc failed: ");
		v_free(host_addr);
		v_free(ls);
		v_free(buf);
		v_free(ev);
		return NULL;
	}
	*head = NULL;

	v_memzero(ls, sizeof(v_listening_t));
	v_memzero(ev, sizeof(v_io_event_t));

	if (v_io_prepare(ev) == V_ERR) {
		v_free(ls);
		v_free(buf);
		v_free(ev);
		v_free(host_addr);
		return NULL;
	}

	ls->event = ev;
	ls->buffer = buf;
	ev->data = ls;
	ls->head = head;

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

static int
handle_events(v_io_event_t *ev)
{
	return V_OK;
}

int
v_connection_read(v_connection_t *s, v_ssize_t size, v_method_t *callback)
{
	v_string_t      *data;
	if (v_stream_length(s->buf) >= size) {
		data = v_stream_read(s->buf, size);
		if (data == NULL) {
			v_close_connection(s);
			return V_ERR;
		}
		return v_method_call(callback, 1, data);
	}
	s->read_count = size;
	s->read_callback = callback;
	return v_io_add(s->event, V_IO_READ, handle_events);
}

int
v_connection_read_until(v_connection_t *s, char *delimeter, v_ssize_t size, v_ssize_t max, v_method_t *callback)
{
	v_string_t      *data;
	int             res;

	res = v_stream_read_until(s->buf, &data, delimeter, size, max);
	if (res == V_OK) {
		return v_method_call(callback, 1, data);
	} else if (res == V_AGAIN) {
		s->read_callback = callback;
		return v_io_add(s->event, V_IO_READ, handle_events);
	} 
	/* V_ERR */
	v_close_connection(s);
	return V_ERR;
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

	v_config.free_connections = c->next;
	v_config.free_connections_count--;

	v_io_prepare(c->event);
	c->event->data = c;

	return c;
}

void
v_connection_free(v_connection_t *c)
{
	v_free(c->local_addr);
	v_free(c->remote_addr);
	v_free(c->read_callback);
	v_free(c->write_callback);
	v_free(c);
}

void
v_connection_idle(v_connection_t *c)
{
	c->next = v_config.free_connections;
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
			v_remove_from_accepted_list(c);
			v_connection_idle(c);
			return;
		default:
			v_log(V_LOG_ALERT, "Close a connection in Unknown status: %d", ev->status);
			return;
	}
}

