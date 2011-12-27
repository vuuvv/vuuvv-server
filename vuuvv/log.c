#include "vuuvv.h"

typedef struct _v_config_s {
	int         verbosity;
	char        *logfile;
} v_config_s;

static v_config_s config = {
	V_LOG_DEBUG,
	NULL
};

void
v_log(int level, const char *fmt, ...)
{
	const char      *c = ".-*#";
	time_t          now = time(NULL);
	va_list         ap;
	FILE            *fp;
	char            buf[64];
	char            msg[V_MAX_LOGMSG_LEN];
	char            *logfile = config.logfile;

	if (level < config.verbosity) return;

	fp = (logfile == NULL) ? stdout : fopen(logfile, "a");
	if (!fp) {
		config.logfile = NULL;
		v_log(V_LOG_WARN, "Open log file '%s' failed: %s", 
			logfile, v_strerror(v_errno, msg, V_MAX_LOGMSG_LEN));
		return;
	}

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	strftime(buf, sizeof(buf), "%d %b %H:%M:%S", localtime(&now));
	fprintf(fp, "[%d] %s %c %s\n", (int)v_getpid(), buf, c[level], msg);
	fflush(fp);

	if (logfile) fclose(fp);
}

void
v_log_error(int level, v_err_t err, const char *fmt, ...)
{
	const char      *c = ".-*#";
	time_t          now = time(NULL);
	va_list         ap;
	FILE            *fp;
	char            buf[64];
	char            msg[V_MAX_LOGMSG_LEN];
	char            *logfile = config.logfile;
	int             size;

	if (level < config.verbosity) return;

	fp = (logfile == NULL) ? stdout : fopen(logfile, "a");
	if (!fp) {
		config.logfile = NULL;
		v_log(V_LOG_WARN, "Open log file '%s' failed: %s", 
			logfile, v_strerror(v_errno, msg, V_MAX_LOGMSG_LEN));
		return;
	}

	va_start(ap, fmt);
	size = vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	v_strerror(err, &msg[size], V_MAX_LOGMSG_LEN - size);
	strftime(buf, sizeof(buf), "%d %b %H:%M:%S", localtime(&now));
	fprintf(fp, "[%d] %s %c %s\n", (int)v_getpid(), buf, c[level], msg);
	fflush(fp);

	if (logfile) fclose(fp);
}

