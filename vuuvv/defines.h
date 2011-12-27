#define  V_OK          0
#define  V_ERR        -1
#define  V_AGAIN      -2
#define  V_BUSY       -3
#define  V_DONE       -4
#define  V_DECLINED   -5
#define  V_ABORT      -6

/* Log levels */
#define V_LOG_DEBUG     0
#define V_LOG_INFO      1
#define V_LOG_WARN      2
#define V_LOG_ERROR     3

#define LF     (u_char) 10
#define CR     (u_char) 13
#define CRLF   "\x0d\x0a"

#define  V_MAX_LOGMSG_LEN 2048

#define v_abs(value)       (((value) >= 0) ? (value) : - (value))
#define v_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))
#define v_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))

