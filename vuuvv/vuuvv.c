#include "vuuvv.h"
#include <time.h>
#include <process.h>

typedef struct _v_config_s {
	int         verbosity;
	char        *logfile;
} v_config_s;

static v_config_s config = {
	V_LOG_DEBUG,
	"./kdjfkdls/dsklfjlsd/sdlfjlsf/vuuvv.log"
};

PyDoc_STRVAR(vuuvv_doc,
"Python vuuvv module");

void v_log(int level, const char *fmt, ...)
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

void v_log_error(int level, v_err_t err, const char *fmt, ...)
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

static PyObject *
vuuvv_test(PyObject *self, PyObject *args, PyObject *kwds)
{
	_PyTime_timeval tp;
	_PyTime_gettimeofday(&tp);
	printf("%d, %d\n", tp.tv_sec, tp.tv_usec);
	v_log(V_LOG_WARN, "hi");
	v_log_error(V_LOG_WARN, 2, "error %d: ", 2);
	Py_RETURN_NONE;
}

static struct PyMethodDef vuuvv_methods[] = {
	{"test", (PyCFunction)vuuvv_test, METH_VARARGS | METH_KEYWORDS, 
		"vuuvv test function"},
	//{"listremove", (PyCFunction)pygui_listremove, METH_VARARGS | METH_KEYWORDS, 
	//	"remove elements from a list"},
	{NULL, NULL}
};

static PyModuleDef vuuvv_module = {
	PyModuleDef_HEAD_INIT,
	"vuuvv",
	vuuvv_doc,
	-1,
	vuuvv_methods,
};

PyMODINIT_FUNC
PyInit_vuuvv(void)
{
	PyObject *m;

	/*
	if (PyType_Ready(&v_event_t) < 0)
		return NULL;
	if (PyType_Ready(&vtask_t) < 0)
		return NULL;
	if (PyType_Ready(&IoData_Type) < 0)
		return NULL;
	if (PyType_Ready(&Connector_Type) < 0)
		return NULL;
	*/

	m = PyModule_Create(&vuuvv_module);
	if (m == NULL)
		return NULL;

	/*
	Py_INCREF(&v_event_t);
	PyModule_AddObject(m, "event", (PyObject *)&v_event_t);
	Py_INCREF(&vtask_t);
	PyModule_AddObject(m, "vtask", (PyObject *)&vtask_t);
	Py_INCREF(&IoData_Type);
	PyModule_AddObject(m, "iodata", (PyObject *)&IoData_Type);
	Py_INCREF(&Connector_Type);
	PyModule_AddObject(m, "connector", (PyObject *)&Connector_Type);
	*/

	return m;
}
