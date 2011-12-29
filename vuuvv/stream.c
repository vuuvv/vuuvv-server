#include "vuuvv.h"

v_inline(int)
_length(v_stream_t stream)
{
	return stream->pos - stream->head;
}

v_inline(int)
_space(v_stream_t stream)
{
	return stream->alloc - stream->pos;
}

static int
v_stream_resize(v_stream_t *stream, size_t size)
{
	char *new_buf = NULL;
	size_t alloc = stream->alloc;

	if (size > V_SSIZE_T_MAX) {
		v_log(V_LOG_ALERT, "New buffer size too large");
		return V_ERR;
	}

	if (size < alloc / 2)
		alloc = size + 1;
	else if (size < alloc)
		return V_OK;
	else if (size <= alloc * 1.125)
		alloc = size + (size >> 3) + (size < 9 ? 3 : 6);
	else
		alloc = size + 1;

	if (alloc > V_SSIZE_T_MAX) {
		v_log(V_LOG_ALERT, "New buffer size too large");
		return V_ERR;
	}

	new_buf = (char *)v_realloc(stream->buf, alloc);
	if (new_buf == NULL) {
		v_log_error(V_LOG_ALERT, v_errno, "Alloc memory failed: ");
		return V_ERR;
	}

	stream->alloc = alloc;
	stream->buf = new_buf;

	return V_OK;
}

v_stream_t *
v_stream_new()
{
}

void
v_stream_reset(v_stream_t *stream)
{
	stream->head = 0;
	stream->pos = 0;
	stream->last_find_pos = 0;
}

int
v_stream_write(v_stream_t stream, const char *byte, v_ssize_t len)
{
	if (len > V_STREAM_SPACE(stream)) {
		if (stream->head + _space(stream) >= len) {
			memcpy(stream->buf, stream->buf + stream->head, _length(stream));
			stream->pos -= stream->head;
			stream->head = 0;
		} else {
			if (_stream_resize(stream, (size_t)stream->pos + len) < 0)
				return -1;
		}
	}

	memcpy(stream->buf + stream->pos, bytes, len);
	stream->pos += len;

	return len;
}

char *
v_stream_read(v_stream_s *stream, Py_ssize_t size)
{
	char *output;

	assert(stream->buf != NULL);

	if (size < 0 || size > V_STREAM_LENGTH(stream)) {
		size = V_STREAM_LENGTH(stream);
	}

	output = stream->buf + stream->head;
	stream->head += size;

	if (stream->last_find_pos >= size) {
		stream->last_find_pos -= size;
	}

	return PyBytes_FromStringAndSize(output, size);
}

