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

