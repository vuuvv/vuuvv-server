#include "vuuvv.h"

static int v_io_accept(v_io_event_t *ev);
static int v_io_connect(v_io_event_t *ev);
static int v_io_read(v_io_event_t *ev);
static int v_io_write(v_io_event_t *ev);
static int v_io_close(v_io_event_t *ev);

static void handle_accept(v_io_event_t *ev);
static void handle_connect(v_io_event_t *ev);
static void handle_read(v_io_event_t *ev);
static void handle_write(v_io_event_t *ev);
static void handle_close(v_io_event_t *ev);

/*
 * function: poll, get the event to handle.
 * function: add_event, add a event to be polled.
 * 
 */

static HANDLE iocp;

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

/**
 * Turn io status(READY, CLOSED, USABLE) to READY. If event in
 * other status, failed.
 * Parameters:
 *  ev[in/out:v_io_event_t]:
 *      the event for prepare.
 * Returns:
 *  If success return V_OK, else V_ERR.
 * Throws:
 *  None.
 * Remarks:
 *  If failed, we close the socket and release the resource.
 **/

int
v_io_prepare(v_io_event_t *ev)
{
	v_socket_t      fd;

	switch(ev->status) {
		case V_IO_STATUS_READY:
		case V_IO_STATUS_ESTABLISHED:
		case V_IO_STATUS_LISTENING:
			return V_OK;
		case V_IO_STATUS_CLOSED:
			fd = v_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (fd == INVALID_SOCKET) {
				v_log_error(V_LOG_ALERT, v_socket_errno, v_socket_name " failed: ");
				return V_ERR;
			}
			ev->fd = fd;
			ev->status = V_IO_STATUS_USABLE;
		case V_IO_STATUS_USABLE:
			if (CreateIoCompletionPort((HANDLE) ev->fd, iocp, (ULONG_PTR)ev, 0) == NULL) {
				v_log_error(V_LOG_ERROR, v_errno, "CreateIoCompletionPort() failed: ");
				return V_ERR;
			}
			ev->status = V_IO_STATUS_READY;
			return V_OK;
		default:
			v_log(V_LOG_ALERT, "v_io_prepare() failed: io status unexpected, %d", ev->status);
			return V_ERR;
	}
}

int
v_io_poll()
{
	int             res, n;
	v_io_event_t    *ev;
	OVERLAPPED      *ovlp;

	res = GetQueuedCompletionStatus(iocp, &n, (PULONG_PTR)&ev, &ovlp, 10);

	if (res) {
		v_log(V_LOG_DEBUG, "Get Event %d, %d bytes", ev->type, n);
		switch (ev->type) {
			case V_IO_ACCEPT:
				handle_accept(ev);
				break;
			case V_IO_CONNECT:
				handle_connect(ev);
				break;
			case V_IO_READ:
				handle_read(ev);
				break;
			case V_IO_WRITE:
				handle_write(ev);
				break;
			case V_IO_CLOSE:
				handle_close(ev);
				break;
		}
	}

	return res;
}

int
v_io_add(v_io_event_t *ev, int event, v_io_proc handler)
{
	ev->type = event;
	ev->handler = handler;

	v_log(V_LOG_DEBUG, "io add %d", event);
	switch(event) {
		case V_IO_ACCEPT:
			return v_io_accept(ev);
		case V_IO_CONNECT:
			return v_io_connect(ev);
		case V_IO_READ:
			return v_io_read(ev);
		case V_IO_WRITE:
			return v_io_write(ev);
		case V_IO_CLOSE:
			return v_io_close(ev);
		default:
			v_log(V_LOG_ERROR, "Use the unknown event type: %d", event);
			return V_ERR;
	}

	return V_OK;
}

static int
v_io_accept(v_io_event_t *ev)
{
	v_listening_t   *ls;
	v_connection_t  *c;
	size_t          n;
	int             err, res;

	assert(ev->status == V_IO_STATUS_LISTENING);

	ls = ev->data;

	c = v_get_connection();
	if (c == NULL) {
		return V_ERR;
	}

	ls->connection = c;
	c->head = ls->head;
	v_log(V_LOG_DEBUG, "before add to accepted list %x", c->head);
	v_add_to_accepted_list(c);

	v_memzero(&ev->ovlp, sizeof(ev->ovlp));

	res = v_acceptex(ev->fd, c->event->fd, ls->buffer, 0, 
			sizeof(struct sockaddr) + 16, sizeof(struct sockaddr) + 16,
			&n, (LPOVERLAPPED)&ev->ovlp) == 0;

	if (res == TRUE) {
		ev->status = V_IO_STATUS_STANDBY_L;
		return V_OK;
	}

	err = v_socket_errno;
	if (err == WSA_IO_PENDING) {
		ev->status = V_IO_STATUS_STANDBY_L;
		return V_OK;
	}

	v_log_error(V_LOG_ALERT, err, "AcceptEx() failed: ");
	v_close_connection(c);
	return V_ERR;
}

static void
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

	c->event->status = V_IO_STATUS_ESTABLISHED;
	ev->handler(ev);

	ev->status = V_IO_STATUS_LISTENING;
	if (ev->type = V_IO_ACCEPT) {
		v_io_accept(ev);
	}
}

static int
v_io_connect(v_io_event_t *ev)
{
	int                 n, res, err;
	v_connection_t      *c;
	struct sockaddr_in  local;

	assert(ev->status == V_IO_STATUS_READY);

	c = ev->data;

	local.sin_family = AF_INET;
	local.sin_addr.s_addr = htonl(ADDR_ANY);
	local.sin_port = htons(0);
	if (bind(ev->fd, (SOCKADDR *)&local, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
		v_log_error(V_LOG_ALERT, v_socket_errno, "bind() in ConnectEx failed: ");
		v_close_connection(c);
		return V_ERR;
	}

	v_memzero(&ev->ovlp, sizeof(ev->ovlp));
	res = v_connectex(ev->fd, (struct sockaddr *)c->remote_addr, sizeof(struct sockaddr), 
			NULL, 0, &n, &ev->ovlp);

	if (res == TRUE) {
		ev->status = V_IO_STATUS_CONNECTING;
		return V_OK;
	}

	err = v_socket_errno;
	if (err == WSA_IO_PENDING) {
		ev->status = V_IO_STATUS_CONNECTING;
		return V_OK;
	}

	v_log_error(V_LOG_ALERT, err, "Connectex() failed: ");
	v_close_connection(c);
	return V_ERR;
}

static void
handle_connect(v_io_event_t *ev)
{
	int  res, n;  
	v_connection_t  *c;

	c = ev->data;
	res = setsockopt(ev->fd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
	if (res == INVALID_SOCKET) {
		v_log_error(V_LOG_ALERT, v_socket_errno, "setsocketopt(SO_UPDATE_CONNECT_CONTEXT) failed: ");
	}

	c->local_addr = v_new(struct sockaddr);
	if (c->local_addr != NULL) {
		n = sizeof(struct sockaddr);
		getsockname(ev->fd, (struct sockaddr *)c->local_addr, &n);
	} else {
		v_log_error(V_LOG_ALERT, v_errno, "Alloc memory failed: ");
	}

	ev->status = V_IO_STATUS_ESTABLISHED;
	ev->handler(ev);
}

static int
v_io_read(v_io_event_t *ev)
{
	int                 err, res, n, flags = 0;
	v_connection_t      *c;
	WSABUF              wbuf;

	assert(ev->status == V_IO_STATUS_ESTABLISHED);

	c = ev->data;
	v_memzero(&ev->ovlp, sizeof(ev->ovlp));
	wbuf.buf = NULL;
	wbuf.len = 0;

	res = WSARecv(ev->fd, &wbuf, 1, &n, &flags, (LPOVERLAPPED)&ev->ovlp, NULL);

	if (res == 0) {
		ev->status = V_IO_STATUS_STANDBY_C;
		return V_OK;
	}

	err = v_errno;
	if (err == ERROR_IO_PENDING) {
		ev->status = V_IO_STATUS_STANDBY_C;
		return V_OK;
	}

	v_log_error(V_LOG_ALERT, err, "WSARecv failed: ");
	v_close_connection(c);
	return V_ERR;
}

/**
 *  If you the length of read buffer is zero, mean socket closed by peer.
 *  You should check this at event handler function.
 **/
static void
handle_read(v_io_event_t *ev)
{
	ev->handler(ev);
	ev->status = V_IO_STATUS_READY;
	if (ev->type == V_IO_READ)
		v_io_read(ev);
}

static int
v_io_write(v_io_event_t *ev)
{
	int                 err, res, n, flags = 0;
	v_connection_t      *c;
	WSABUF              wbuf;

	assert(ev->status == V_IO_STATUS_ESTABLISHED);

	c = ev->data;
	v_memzero(&ev->ovlp, sizeof(ev->ovlp));
	wbuf.buf = c->write_bytes->data;
	wbuf.len = c->write_bytes->len;

	res = WSASend(ev->fd, &wbuf, 1, &n, 0, (LPOVERLAPPED)&ev->ovlp, NULL);

	if (res == 0) {
		v_log(V_LOG_DEBUG, "Write Immediatly");
		ev->status = V_IO_STATUS_STANDBY_L;
		return V_OK;
	}

	err = v_errno;
	if (err == ERROR_IO_PENDING) {
		ev->status = V_IO_STATUS_STANDBY_L;
		return V_OK;
	}

	v_log_error(V_LOG_ALERT, err, "WSASend failed: ");
	v_close_connection(c);
	return V_ERR;
}

static void
handle_write(v_io_event_t *ev)
{
	ev->handler(ev);
	ev->status = V_IO_STATUS_READY;
	if (ev->type == V_IO_WRITE)
		v_io_read(ev);
}

/**
 * This function is only for connection, not for listening.
 **/
static int
v_io_close(v_io_event_t *ev)
{
	int                 err, res;
	v_connection_t      *c;

	c = ev->data;
	v_memzero(&ev->ovlp, sizeof(ev->ovlp));

	v_remove_from_accepted_list(c);
	res = v_disconnectex(ev->fd, (LPOVERLAPPED)&ev->ovlp, TF_REUSE_SOCKET, 0);

	if (res == 0) {
		v_log(V_LOG_DEBUG, "Close Immediatly");
		ev->status = V_IO_STATUS_CLOSING;
		return V_OK;
	}

	err = v_errno;
	if (err == ERROR_IO_PENDING) {
		ev->status = V_IO_STATUS_CLOSING;
		return V_OK;
	}

	v_log_error(V_LOG_ALERT, err, "Disconnectex failed: ");
	return V_ERR;
}

static void
handle_close(v_io_event_t *ev)
{
	v_log(V_LOG_DEBUG, "handle close");
	ev->handler(ev);
	v_connection_idle(ev->data);
	ev->status = V_IO_STATUS_READY;
}

