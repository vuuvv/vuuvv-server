#include "vuuvv.h"

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
		PyErr_Format(PyExc_RuntimeError,
				"WSAStartup() failed: %d\n", res);
		return -1;
	}

	if (iocp == NULL) {
		iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	}

	if (iocp == NULL) {
		PyErr_Format(PyExc_RuntimeError, "CreateIoCompletionPort() failed");
		return -1;
	}

	dummy = v_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	for (i = 0; i < LPFN_COUNT; i++) {
		res = WSAIoctl(dummy, SIO_GET_EXTENSION_FUNCTION_POINTER,
				guid[i], sizeof(GUID),
				fn[i], sizeof(LPFN_ACCEPTEX),
				&n, NULL, NULL);
		if (res) {
			PyErr_Format(PyExc_RuntimeError,
					"io init failed: %d", res);
			closesocket(dummy);
			return -1;
		}
	}

	closesocket(dummy);

	return 0;
}