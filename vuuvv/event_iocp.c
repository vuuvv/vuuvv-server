#include "vuuvv.h"

static int v_io_accept(v_io_event_t *ev);
static int v_io_connect(v_io_event_t *ev);
static int v_io_read(v_io_event_t *ev);
static int v_io_write(v_io_event_t *ev);

static int handle_accept(v_io_event_t *ev);
static int handle_connect(v_io_event_t *ev);
static int handle_read(v_io_event_t *ev);
static int handle_write(v_io_event_t *ev);

static void v_close_prepare_connection(v_connection_t *c);
/*
 * function: poll, get the event to handle.
 * function: add_event, add a event to be polled.
 * 
 */

static HANDLE iocp;

v_inline(int)
v_prepare_ev(v_io_event_t *ev)
{
	if (!ev->ready && CreateIoCompletionPort((HANDLE) ev->fd, iocp, (ULONG_PTR)ev, 0) == NULL) {
		v_log_error(V_LOG_ERROR, v_errno,
				"CreateIoCompletionPort() failed: ");
		return V_ERR;
	}
	ev->ready = 1;
	return V_OK;
}

v_inline(void)
v_shutdown_socket(fd)
{
	if (v_close_socket((fd)) == INVALID_SOCKET) {
		v_log_error(V_LOG_ERROR, v_socket_errno, v_close_socket_name " failed: ");
	}      
}

#define LPFN_COUNT 6
static LPFN_ACCEPTEX              v_acceptex;
static LPFN_GETACCEPTEXSOCKADDRS  v_getacceptexsockaddrs;
static LPFN_TRANSMITFILE          v_transmitfile;
static LPFN_TRANSMITPACKETS       v_transmitpackets;
static LPFN_CONNECTEX             v_connectex;
static LPFN_DISCONNECTEX          v_disconnectex;

static GUID ax_guid = WSAID_ACCEPTEX;
static GUID as_guid = WSAID_GETACCEPTEXSOCKADDRS;
static GUID tf_guid = WSAID_TRANSMITFILE;
static GUID tp_guid = WSAID_TRANSMITPACKETS;
static GUID cx_guid = WSAID_CONNECTEX;
static GUID dx_guid = WSAID_DISCONNECTEX;

/**
 * Init the io evironment.
 * Parameters:
 *      No parameters.
 * Returns:
 *      If success return 0, else -1.
 * Throws:
 *      RuntimeError - Raised if init failure.
 **/
int
v_io_init(void)
{
	LPVOID fn[LPFN_COUNT] = {
		&v_acceptex,
		&v_getacceptexsockaddrs,
		&v_transmitfile,
		&v_transmitpackets,
		&v_connectex,
		&v_disconnectex
	};
	LPVOID guid[LPFN_COUNT] = {
		&ax_guid,
		&as_guid,
		&tf_guid,
		&tp_guid,
		&cx_guid,
		&dx_guid
	};
	int         i, res, n;
	v_socket_t  dummy = INVALID_SOCKET;
	WSADATA     data;

	res = WSAStartup(MAKEWORD(2, 2), &data);
	if (res) {
		v_log_error(V_LOG_ERROR, v_socket_errno, "WASStarup() failed: ");
		return V_ERR;
	}

	if (iocp == NULL) {
		iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	}

	if (iocp == NULL) {
		v_log_error(V_LOG_ERROR, v_errno, "CreateIoCompletionPort() failed: ");
		return V_ERR;
	}

	dummy = v_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	for (i = 0; i < LPFN_COUNT; i++) {
		res = WSAIoctl(dummy, SIO_GET_EXTENSION_FUNCTION_POINTER,
				guid[i], sizeof(GUID),
				fn[i], sizeof(LPFN_ACCEPTEX),
				&n, NULL, NULL);
		if (res) {
			v_log_error(V_LOG_ERROR, v_socket_errno, "WSAIoctl failed: ");
			closesocket(dummy);
			return V_ERR;
		}
	}

	closesocket(dummy);

	return V_OK;
}

int
v_io_poll()
{
	int             res, n;
	v_io_event_t    *ev;
	OVERLAPPED      *ovlp;

	res = GetQueuedCompletionStatus(iocp, &n, (PULONG_PTR)&ev, &ovlp, 50);

	if (res) {
		v_log(V_LOG_INFO, "Get Event %d", ev->type);
		switch (ev->type) {
			case V_IO_ACCEPT:
				res = handle_accept(ev);
				break;
			case V_IO_CONNECT:
				res = handle_connect(ev);
				break;
			case V_IO_READ:
				res = handle_read(ev);
				break;
			case V_IO_WRITE:
				res = handle_write(ev);
				break;
		}
	}

	return res;
}

int
v_io_add(v_io_event_t *ev, int event, v_io_proc handler)
{
	if (v_prepare_ev(ev) == V_ERR) {
		return V_ERR;
	}
	ev->type = event;
	ev->handler = handler;

	switch(event) {
		case V_IO_ACCEPT:
			return v_io_accept(ev);
		case V_IO_CONNECT:
			return v_io_connect(ev);
		case V_IO_READ:
			return v_io_read(ev);
		case V_IO_WRITE:
			return v_io_write(ev);
		default:
			v_log(V_LOG_ERROR, "Use the unknown event type: %d", event);
			return V_ERR;
	}

	return V_OK;
}

static int
v_io_accept(v_io_event_t *ev)
{
	v_socket_t      fd;
	v_listening_t   *ls;
	v_connection_t  *c;
	size_t          n;
	int             err;

	ls = ev->data;

	fd = v_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == INVALID_SOCKET) {
		v_log_error(V_LOG_ALERT, v_socket_errno, v_socket_name " failed: ");
		return V_ERR;
	}

	ls->buffer = v_malloc(2 * sizeof(struct sockaddr) + 16);
	if (ls->buffer == NULL) {
		v_log_error(V_LOG_ALERT, v_errno, "v_malloc failed: ");
		return V_ERR;
	}

	c = v_get_connection(fd);
	if (c == NULL) {
		v_shutdown_socket(fd);
		return V_ERR;
	}

	ls->connection = c;
	c->listening = ls;

	v_memzero(&ev->ovlp, sizeof(ev->ovlp));

	if (v_acceptex(ev->fd, fd, ls->buffer, 0, 
			sizeof(struct sockaddr) + 16, sizeof(struct sockaddr) + 16,
			&n, (LPOVERLAPPED)&ev->ovlp) == 0) {
		err = v_socket_errno;
		if (err != WSA_IO_PENDING) {
			v_log_error(V_LOG_ALERT, err, "AcceptEx() failed: ");
			v_close_prepare_connection(c);
			return V_ERR;
		}
	}

	return V_OK;
}

void
handle_accept(v_io_event_t *ev)
{
	v_listening_t       *ls;
	v_connection_t      *c;
	int                 ln, rn;

	ls = ev->data;
	c = ls->connection;

	if (setsockopt(c->event->fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
				(char *) &ls->event->fd, sizeof(v_socket_t)) == INVALID_SOCKET) {
		v_log_error(V_LOG_ALERT, v_socket_errno, "setsocketopt(SO_UPDATE_ACCEPT_CONTEXT) failed: ");
	}

	v_getacceptexsockaddrs(ls->buffer, 0,
			sizeof(struct sockaddr) + 16, sizeof(struct sockaddr) + 16,
			(LPSOCKADDR *)&c->remote_addr, &rn,
			(LPSOCKADDR *)&c->local_addr, &ln);

	v_io_accept(ev);

	ev->handler(c->event);
}

static int
v_io_connect(v_io_event_t *ev)
{
	return V_OK;
}

static int
handle_connect(v_io_event_t *ev)
{
	return V_OK;
}

static int
v_io_read(v_io_event_t *ev)
{
	return V_OK;
}

static int
handle_read(v_io_event_t *ev)
{
	return V_OK;
}

static int
v_io_write(v_io_event_t *ev)
{
	return V_OK;
}

static int
handle_write(v_io_event_t *ev)
{
	return V_OK;
}

static void
v_close_prepare_connection(v_connection_t *c)
{
	v_socket_t  fd;

	v_free_connection(c);

	fd = c->event->fd;
	c->event->fd = (v_socket_t) -1;

	if (v_close_socket(fd) == -1) {
		v_log_error(V_LOG_ERROR, v_socket_errno,
				v_close_socket_name " failed: ");
	}
}
