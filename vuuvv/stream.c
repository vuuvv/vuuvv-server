#include "vuuvv.h"

#include "stringlib/stringdefs.h"
#include "stringlib/fastsearch.h"

/**
 * We suppose when we call readuntil(), the read bytes should not be long,
 * so we don't remember the last find position, and find a string from head,
 * every time.
 **/
#define V_STREAM_INIT_SIZE 256

v_inline(int)
_length(v_stream_t *stream)
{
	return stream->pos - stream->head;
}

v_inline(int)
_space(v_stream_t *stream)
{
	return stream->alloc - stream->pos;
}

v_inline(int)
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

v_inline(int)
v_stream_find(v_stream_t *stream, v_ssize_t start, char *buf, v_ssize_t size)
{
	v_ssize_t   len, pos;

	len = _length(stream);
	if (start > len) {
		return -1;
	}

	start = start > size ? start - size : 0;
	len -= start;

	pos = fastsearch(stream->buf + stream->head + start, len, buf, size, -1, FAST_SEARCH);

	return pos >= 0 ? pos + start : pos;
}

v_stream_t *
v_stream_new()
{
	v_stream_t *stream = v_new(v_stream_t);

	if (stream == NULL) {
		v_log_error(V_LOG_ALERT, v_errno, "Alloc memory failed");
		return NULL;
	}

	stream->head = 0;
	stream->pos = 0;
	stream->alloc = V_STREAM_INIT_SIZE;
	stream->buf = v_malloc(V_STREAM_INIT_SIZE);
	if (stream->buf == NULL) {
		v_log_error(V_LOG_ALERT, v_errno, "Alloc memory failed");
		v_free(stream);
		return NULL;
	}

	return stream;
}

void
v_stream_free(v_stream_t *stream)
{
	v_free(stream->buf);
	v_free(stream);
}

void
v_stream_reset(v_stream_t *stream)
{
	stream->head = 0;
	stream->pos = 0;
}

int
v_stream_write(v_stream_t *stream, const char *bytes, v_ssize_t len)
{
	if (len > _space(stream)) {
		if (stream->head + _space(stream) >= len) {
			memcpy(stream->buf, stream->buf + stream->head, _length(stream));
			stream->pos -= stream->head;
			stream->head = 0;
		} else {
			if (v_stream_resize(stream, (size_t)stream->pos + len) < 0)
				return -1;
		}
	}

	memcpy(stream->buf + stream->pos, bytes, len);
	stream->pos += len;

	return len;
}

v_string_t *
v_stream_read(v_stream_t *stream, v_ssize_t size)
{
	char        *buf;
	v_string_t  *result;

	buf = v_malloc(size);
	if (buf == NULL) {
		v_log_error(V_LOG_ALERT, v_errno, "Alloc memory failed");
		return NULL;
	}
	result = v_string(buf, size);
	if (result == NULL) {
		v_free(buf);
		return NULL;
	}

	if (size < 0 || size > _length(stream)) {
		size = _length(stream);
	}

	memcpy(buf, stream->buf + stream->head, size);
	stream->head += size;

	return result;
}

/**
 * Read a string from the stream until meet the delimeter.
 * Parameters:
 *  stream[in:v_stream_t]:
 *      The source which we will read string.
 *  str[out:v_string_t *]
 *      The result string.
 *  delimeter[in:char *]:
 *      A string which determind where the read process end.
 *  size[in:v_ssize_t]:
 *      The length of delimeter string.
 *  max[in:v_ssize_t]:
 *      The max size of string we will get.
 * Returns:
 *      If success return V_OK, it max size is specified at parameter
 *      'max'. If we don't find the delimeter and stream length is little than 'max', 
 *      return V_AGAIN. If we don't find the delimeter and stream length is greater than
 *      'max', return V_ERR(mean you should close the relative connection). The bytes
 *      between max and delimeter pos will be discard.
 * Throws:
 *      None.
 **/
int
v_stream_read_until(v_stream_t *stream, v_string_t **str, char *delimeter, v_ssize_t size, v_ssize_t max)
{
	v_ssize_t   pos, len;

	pos = v_stream_find(stream, 0, delimeter, size);
	if (pos == -1) {
		return max < _length(stream) ? V_ERR : V_AGAIN;
	}
	pos += size;

	len = min(max, pos);
	*str = v_stream_read(stream, len);
	if (*str == NULL) {
		return V_ERR;
	}
	stream->head += pos - max;
	return V_OK;
}


