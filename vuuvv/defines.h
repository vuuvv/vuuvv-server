#define  V_OK          0
#define  V_ERR        -1
#define  V_AGAIN      -2
#define  V_BUSY       -3
#define  V_DONE       -4
#define  V_DECLINED   -5
#define  V_ABORT      -6

#define LF     (u_char) 10
#define CR     (u_char) 13
#define CRLF   "\x0d\x0a"

#define  V_MAX_LOGMSG_LEN 2048

#define v_abs(value)       (((value) >= 0) ? (value) : - (value))
#define v_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))
#define v_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))

#define v_malloc        PyMem_Malloc
#define v_new           PyMem_New
#define v_free          PyMem_Free
#define v_realloc       PyMem_Realloc

#define v_inline        Py_LOCAL_INLINE

#define v_ssize_t       Py_ssize_t
#define V_SSIZE_T_MAX   PY_SSIZE_T_MAX
