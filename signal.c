
static void signal_handler(int sig) {
	switch (sig) {
		case SIGINT:
		case SIGTERM:
		case SIGQUIT:
			printf("\nReceived signal %d, shutting down server...\n", sig);
			server_stop();
			break;
		default:
			break;
	}
}

static void setup_signals(void) {
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGPIPE, SIG_IGN); // Ignore broken pipe signals
}
