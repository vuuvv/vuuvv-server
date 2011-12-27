typedef DWORD                       v_err_t;

#define v_errno                     GetLastError()
#define v_set_errno(err)            SetLastError(err)
#define v_socket_errno              WSAGetLastError()
#define v_socket_set_errno          WSASetLastError(err)

char *v_strerror(v_err_t err, char *errstr, size_t size);
int v_strerror_init(void);

