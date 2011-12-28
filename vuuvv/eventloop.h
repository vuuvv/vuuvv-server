#define V_IO_ACCEPT         0x01
#define V_IO_READ           0x02
#define V_IO_WRITE          0x04
#define V_IO_CONNECT        0x08
#define V_IO_CLOSE          0x10
#define V_IO_CLOSED         0x20
#define V_IO_ERROR          0x40
#define V_IOLOOP_EXIT       0x80

typedef struct v_eventloop_s v_eventloop_t;
typedef struct v_connection_s v_connection_t;

typedef void v_io_proc(v_eventloop_t *loop, int fd, void *data, int flags);
typedef int v_time_proc(v_eventloop_t *loop, long long id, void *data);

typedef struct v_io_event_s {
	void                    *data;
	int                     type;
	v_io_proc               *handler;
} v_io_event_t;

typedef struct v_time_event_s {
	long long               id;
	long                    sec;
	long                    ms;
	v_time_proc             *handler;
	void                    *data;
	struct v_time_event_s   *next;
} v_time_event_t;

typedef struct v_eventloop_s {
	v_time_event_t          *time_event_head;
} v_eventloop_t;

typedef struct v_listening_s {
	v_socket_t              fd;

	struct sockaddr         *sockaddr;
	size_t                  socklen;
	size_t                  addr_text_max_len;
	char                    *addr_text;

	int                     type;
	int                     backlog;

	v_connection_t          *connection;
	v_io_event_t            *event;
	char                    *buffer;
} v_listening_t;

struct v_connection_s{
	v_socket_t              fd;
	v_io_event_t            *event;
	v_listening_t           *listening;
	void                    *data;
};

extern int v_eventloop_init();
extern v_connection_t * v_get_connection(v_socket_t fd);
