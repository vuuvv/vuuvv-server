typedef int (*read_proc)(v_stream_t *stream, v_string_t *bytes);

struct v_stream_s {
	v_ssize_t       alloc;
	v_ssize_t       pos;
	v_ssize_t       head;
	char            *buf;
};

struct v_io_stream_s {
	v_stream_t      *stream;
	v_connection_t  *connection;

	v_ssize_t       read_count;
	v_string_t      *read_delimeter;
	read_proc       read_callback;
}
