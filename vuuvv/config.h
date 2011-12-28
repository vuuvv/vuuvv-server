typedef struct v_config_s {
	int                 verbosity;
	char                *logfile;
	size_t              connections_count;
	v_connection_t      *connections;
	v_connection_t      *free_connections;
	size_t              free_connections_count;
} v_config_t;

extern v_config_t v_config;
