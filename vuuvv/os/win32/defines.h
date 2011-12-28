#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

typedef SOCKET v_socket_t;

#define v_socket(af, type, proto)                                          \
    WSASocket(af, type, proto, NULL, 0, WSA_FLAG_OVERLAPPED)

#define v_socket_name       "WSASocket()"

#define v_getpid        _getpid
#define v_memzero       ZeroMemory

