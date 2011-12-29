typedef struct v_stream_s {
	v_ssize_t       alloc;
	v_ssize_t       pos;
	v_ssize_t       head;

	v_ssize_t       last_find_pos;

	char            *buf;
} v_stream_t;
