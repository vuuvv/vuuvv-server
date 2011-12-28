#include "vuuvv.h"

typedef struct {
	WSAOVERLAPPED   ovlp;
	v_io_event_t    *event;
	int             error;
} v_event_ovlp_t;

static int v_io_accept(v_io_event_t *ev);
static int v_io_connect(v_io_event_t *ev);
static int v_io_read(v_io_event_t *ev);
static int v_io_write(v_io_event_t *ev);
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
		v_log_error(V_LOG_ERROR, v_errno, "WASStarup() failed: ");
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
			v_log_error(V_LOG_ERROR, v_errno, "WSAIoctl failed: ");
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
}

int
v_io_add(v_io_event_t *ev, int event, int key)
{
	v_connection_t *c = ev->data;

	if (CreateIoCompletionPort((HANDLE) c->fd, iocp, key, 0) == NULL) {
		v_log_error(V_LOG_ERROR, v_errno,
				"CreateIoCompletionPort() failed: ");
		return V_ERR;
	}

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
	size_t          len;

	ls = ev->data;
	fd = v_socket(ls->sockaddr->sa_family, ls->type, 0);

	if (fd == INVALID_SOCKET) {
		v_log(V_LOG_ERROR, v_errno, v_socket_name " failed: ");
		return V_ERR;
	}

	c = v_get_connection(fd);

	if (c == NULL) {
		return V_ERR;
	}


	return V_OK;
}

static int
v_io_connect(v_io_event_t *ev)
{
	return V_OK;
}

static int
v_io_read(v_io_event_t *ev)
{
	return V_OK;
}

static int
v_io_write(v_io_event_t *ev)
{
	return V_OK;
}

