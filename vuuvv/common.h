struct v_string_s {
    size_t      len;
    char        *data;
};

#define v_null_string       { 0, NULL }

v_inline(v_string_t *)
v_string(char *str, v_ssize_t len)
{
	v_string_t  *s = v_new(v_string_t);
	if (s == NULL) {
		v_log_error(V_LOG_ALERT, v_errno, "Alloc memory failed");
		return NULL;
	}
	s->data = str;
	s->len = len;
	return s;
}

v_inline(void)
v_string_free(v_string_t *s)
{
	v_free(s->data);
	v_free(s);
}

typedef int (*v_method_proc)(void *context, void *args, size_t size);

struct v_method_s {
	v_method_proc   handler;
	void            *context;
	void            *args;
	size_t          size;
};

v_inline(v_method_t *)
v_method(v_method_proc handler, void *context)
{
	v_method_t  *m = v_new(v_method_t);
	if (m == NULL) {
		v_log_error(V_LOG_ALERT, v_errno, "Alloc memory failed");
		return NULL;
	}
	m->handler = handler;
	m->context = context;
	m->args = NULL;
	m->size = 0;
}

v_inline(void *)
v_build_args(va_list args, size_t size)
{
	void        **result;
	size_t      i;

	result = v_new_n(void *, size);
	if (result == NULL) {
		v_log_error(V_LOG_ALERT, v_errno, "Alloc memory failed");
		return NULL;
	}
	for (i = 0; i < size; i++) {
		result[i] = va_arg(args, void *);
	}

	return result;
}

v_inline(int)
v_method_call(v_method_t *m, size_t size, ...)
{
	va_list     vargs;
	void        **args;
	int         res;

	if (size == 0) {
		return m->handler(m->context, NULL, 0);
	}
	va_start(vargs, size);
	args = v_build_args(vargs, size);
	if (args == NULL) {
		return V_ERR;
	}
	va_end(vargs);
	res = m->handler(m->context, args, size);
	v_free(args);
	return res;
}

