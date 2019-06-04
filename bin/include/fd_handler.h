#ifndef FD_HANDLER_H
#define FD_HANDLER_H

typedef struct fd_handler {
	void (*handle_read)(struct selector_key *key);
	void (*handle_write)(struct selector_key *key);
	void (*handle_block)(struct selector_key *key);

	/**
	 * llamado cuando se se desregistra el fd
	 * Seguramente deba liberar los recusos alocados en data.
	 */
	void (*handle_close)(struct selector_key *key);

} fd_handler;

#endif //FD_HANDLER_H
