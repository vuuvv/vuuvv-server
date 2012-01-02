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

#define v_malloc            PyObject_Malloc
#define v_free              PyObject_Free
#define v_realloc           PyObject_Realloc
#define v_new(type)         v_malloc(sizeof(type))
#define v_new_n(type, n)    v_malloc(sizeof(type) * n)

#define v_inline        Py_LOCAL_INLINE

#define v_ssize_t       Py_ssize_t
#define V_SSIZE_T_MAX   PY_SSIZE_T_MAX


/* typedef */
typedef struct v_string_s       v_string_t;
typedef struct v_method_s       v_method_t; 
typedef struct v_stream_s       v_stream_t;
typedef struct v_io_event_s     v_io_event_t;
typedef struct v_time_event_s   v_time_event_t;
typedef struct v_eventloop_s    v_eventloop_t;
typedef struct v_connection_s   v_connection_t;
typedef struct v_listening_s    v_listening_t;

