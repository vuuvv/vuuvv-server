/* Log levels */
#define V_LOG_DEBUG     0
#define V_LOG_INFO      1
#define V_LOG_WARN      2
#define V_LOG_ERROR     3

extern void v_log(int level, const char *fmt, ...);
extern void v_log_error(int level, v_err_t err, const char *fmt, ...);

