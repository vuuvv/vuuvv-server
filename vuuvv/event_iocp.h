extern int v_io_init(void);
extern int v_io_prepare(v_io_event_t *ev);
extern int v_io_poll();
extern int v_io_add(v_io_event_t *ev, int event, v_io_proc handler);

