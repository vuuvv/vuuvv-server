enum V_STREAM_STATUS {
	V_IOSTREAM_READ_COUNT,
	V_IOSTREAM_READ_UNTIL,
	V_IOSTREAM_WRITE,
};

struct v_stream_s {
	v_ssize_t       alloc;
	v_ssize_t       pos;
	v_ssize_t       head;
	char            *buf;
};

extern v_stream_t * v_stream_new();
extern void v_stream_free(v_stream_t *stream);
extern void v_stream_reset(v_stream_t *stream);
extern int v_stream_write(v_stream_t *stream, const char *bytes, v_ssize_t len);
extern v_string_t *v_stream_read(v_stream_t *stream, v_ssize_t size);
extern int v_stream_read_until(v_stream_t *stream, v_string_t **str, char *delimeter, v_ssize_t size, v_ssize_t max);

v_inline(int)
v_stream_length(v_stream_t *stream)
{
	return stream->pos - stream->head;
}

v_inline(int)
v_stream_space(v_stream_t *stream)
{
	return stream->alloc - stream->pos;
}

