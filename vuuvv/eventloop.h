
typedef struct _v_eventloop_s v_eventloop_s;

typedef void v_io_proc(v_eventloop_s *loop, int fd, void *data, int flags);
typedef int v_time_proc(v_eventloop_s *loop, long long id, void *data);

typedef struct _v_io_event_s {
	int                     flags;
	v_io_proc               *read;
	v_io_proc               *write;
	void                    *data;
} v_io_event_s;

typedef struct _v_time_event_s {
	long long               id;
	long                    sec;
	long                    ms;
	v_time_proc             *time_proc;
	void                    *data;
	struct _v_time_event_s  *next;
} v_time_event_s;

typedef struct _v_eventloop_s {
	v_time_event_s          *time_event_head;
} v_eventloop_s;


