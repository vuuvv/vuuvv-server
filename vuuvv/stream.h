typedef struct v_stream_s v_stream_t;
typedef int (*read_proc)(v_stream_t *stream, char *buf, v_ssize_t len);

struct v_stream_s {
	v_ssize_t       alloc;
	v_ssize_t       pos;
	v_ssize_t       head;

	v_ssize_t       last_find_pos;

	char            *buf;

	v_connection_t  *c;
	v_ssize_t       read_bytes;
	read_proc       read_callback;
};
