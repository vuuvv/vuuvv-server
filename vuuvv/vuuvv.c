#include "vuuvv.h"

PyDoc_STRVAR(vuuvv_doc,
"Python vuuvv module");

static int
test(v_io_event_t *ev)
{
	v_connection_t *c = ev->data;
	printf("test\n");
	v_log(V_LOG_INFO, "OK: %d\n", ntohs(c->remote_addr->sin_port));
	printf("test\n");
	return V_OK;
}

static PyObject *
vuuvv_test(PyObject *self, PyObject *args, PyObject *kwds)
{
	_PyTime_timeval tp;
	v_listening_t *ls;
	_PyTime_gettimeofday(&tp);
	v_eventloop_init();
	v_get_connection(v_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
	printf("%d, %d\n", tp.tv_sec, tp.tv_usec);
	v_io_init();
	ls = v_create_listening(NULL, 9999, 0);
	v_io_add(ls->event, V_IO_ACCEPT, test);
	while(1) {
		v_io_poll();
	}
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
