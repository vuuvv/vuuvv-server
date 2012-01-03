#define V_IO_NONE               0x01
#define V_IO_ACCEPT             0x02
#define V_IO_READ               0x04
#define V_IO_WRITE              0x08
#define V_IO_CONNECT            0x10
#define V_IO_CLOSE              0x20
#define V_IO_SHUTDOWN           0x40

enum V_IO_STATUS {
	V_IO_STATUS_CLOSED,
	V_IO_STATUS_USABLE,
	V_IO_STATUS_READY,
	V_IO_STATUS_ESTABLISHED,
	V_IO_STATUS_LISTENING,
	V_IO_STATUS_CONNECTING,
	V_IO_STATUS_STANDBY_C,
	V_IO_STATUS_STANDBY_L,
	V_IO_STATUS_CLOSING,
};

enum V_CONNECTION_STATUS {
	V_CONNECTION_NONE,
	V_CONNECTION_READ,
	V_CONNECTION_READ_UNTIL,
	V_CONNECTION_WRITE,
};

typedef int (*v_io_proc)(v_io_event_t *ev);
typedef int (*v_time_proc)(v_time_event_t *ev);

struct v_io_event_s {
	v_socket_t              fd;
	void                    *data;
	unsigned short          type;
	unsigned short          status;
	v_io_proc               handler;
#ifdef WIN32
	OVERLAPPED              ovlp;
#endif
};

struct v_time_event_s {
	long long               id;
	long                    sec;
	long                    ms;
	v_time_proc             handler;
	void                    *data;
	struct v_time_event_s   *next;
};

struct v_eventloop_s {
	v_time_event_t          *time_event_head;
};

struct v_listening_s {
	v_io_event_t            *event;

	struct sockaddr_in      *sockaddr;
	size_t                  socklen;
	size_t                  addr_text_max_len;
	char                    *addr_text;

	int                     type;
	int                     backlog;

	v_connection_t          *connection;
	char                    *buffer;

	v_connection_t          **head;
};

struct v_connection_s {
	v_io_event_t            *event;

	struct sockaddr_in      *remote_addr;
	struct sockaddr_in      *local_addr;

	v_stream_t              *buf;

	int                     status;
	v_ssize_t               read_count;
	v_string_t              read_delimeter;
	v_ssize_t               read_until_max;
	v_method_t              *read_callback;

	v_method_t              *write_callback;
	v_string_t              write_bytes;

	v_connection_t          *next;      /* another use: idle list */
	v_connection_t          *prev;
	v_connection_t          **head;     /* head of accept list, null mean not in the list */
};

extern int v_eventloop_init();
extern v_listening_t *v_create_listening(const char *hostname, int port, int backlog);
extern v_connection_t *v_get_connection();
extern void v_connection_idle(v_connection_t *c);
extern int v_connect(const char *hostname, int port, v_io_proc handler);
extern void v_close_connection(v_connection_t *c);
extern int v_connection_read(v_connection_t *s, v_ssize_t size, v_method_t *callback);
extern int v_connection_read_until(v_connection_t *s, char *delimeter, v_ssize_t size, v_ssize_t max, v_method_t *callback);


v_inline(void)
v_add_to_accepted_list(v_connection_t *c)
{
	v_connection_t *h = *(c->head);

	assert(c->head != NULL);    /* Suppose the head value is set when call v_io_accept() */

	v_log(V_LOG_DEBUG, "add accept connection to list");

	if (h == NULL) {
		/* empty */
		c->head = &c;
		c->prev = c;
		c->next = c;
	} else {
		c->next = h;
		c->prev = h->prev;
		h->prev->next = c;
		h->prev = c;
	}
}

v_inline(void)
v_remove_from_accepted_list(v_connection_t *c)
{
	v_log(V_LOG_DEBUG, "remove accept connection from list");

	if (c->head == NULL)
		return;

	if (c->next != c) {
		c->prev->next = c->next;
		c->next->prev = c->prev;
		if (c == *(c->head))
			*(c->head) = c->next;
	}
	c->head = NULL;
}

