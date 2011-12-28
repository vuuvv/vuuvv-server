#include "vuuvv.h"

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
		ev[i].type = V_IO_CLOSED;
	}

	i = connections_count;
	next = NULL;
	do {
		i--;

		c[i].fd = (v_socket_t) -1;
		c[i].data = next;
		c[i].event = &ev[i];

		next = &c[i];

	} while(i);

	v_config.free_connections = next;
	v_config.free_connections_count = connections_count;
	v_config.connections = c;

	return V_OK;
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

	c->fd = fd;

	c->event->data = c;

	return c;
}

void
v_free_connection(v_connection_t *c)
{
	c->data = v_config.free_connections;
	v_config.free_connections = c;
	v_config.free_connections_count++;
}
