#define V_IO_NONE               0x01
#define V_IO_ACCEPT             0x02
#define V_IO_READ               0x04
#define V_IO_WRITE              0x08
#define V_IO_CONNECT            0x10
#define V_IO_CLOSE              0x20
#define V_IO_SHUTDOWN           0x40

/*
#define V_IO_STATUS_NONE        0x00
#define V_IO_STATUS_USABLE      0x01
#define V_IO_STATUS_READY       0x02
#define V_IO_STATUS_STANDBY     0x04
#define V_IO_STATUS_CLOSING     0x08
#define V_IO_STATUS_CLOSED      0x10
#define V_IO_STATUS_ERROR       0x20
*/
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

typedef struct v_io_event_s v_io_event_t;
typedef struct v_time_event_s v_time_event_t;
typedef struct v_eventloop_s v_eventloop_t;
typedef struct v_connection_s v_connection_t;

typedef int (* v_io_proc)(v_io_event_t *ev);
typedef int (* v_time_proc)(v_time_event_t *ev);

typedef struct v_io_event_s {
	v_socket_t              fd;
	void                    *data;
	unsigned short          type;
	unsigned short          status;
	v_io_proc               handler;
#ifdef WIN32
	OVERLAPPED              ovlp;
#endif
};

typedef struct v_time_event_s {
	long long               id;
	long                    sec;
	long                    ms;
	v_time_proc             handler;
	void                    *data;
	struct v_time_event_s   *next;
};

typedef struct v_eventloop_s {
	v_time_event_t          *time_event_head;
} v_eventloop_t;

typedef struct v_listening_s {
	v_io_event_t            *event;

	struct sockaddr_in      *sockaddr;
	size_t                  socklen;
	size_t                  addr_text_max_len;
	char                    *addr_text;

	int                     type;
	int                     backlog;

	v_connection_t          *connection;
	char                    *buffer;
} v_listening_t;

struct v_connection_s{
	v_io_event_t            *event;

	struct sockaddr_in      *remote_addr;
	struct sockaddr_in      *local_addr;
	void                    *data;
};

extern int v_eventloop_init();
extern v_listening_t *v_create_listening(const char *hostname, int port, int backlog);
extern v_connection_t *v_get_connection();
extern void v_free_connection(v_connection_t *c);
extern int v_connect(const char *hostname, int port, v_io_proc handler);
extern void v_close_connection(v_connection_t *c);

