#include <utils/logger.h>
#include "argument.h"
#include <sstream>

namespace bumo {
	bool g_enable_ = true;
	bool g_ready_ = false;
	Argument::Argument() {}
	Argument::~Argument() {}

	void SignalFunc(int32_t code) {
		fprintf(stderr, "Get quit signal(%d)\n", code);
		g_enable_ = false;
	}

	void InstallSignal() {
		signal(SIGHUP, SignalFunc);
		signal(SIGQUIT, SignalFunc);
		signal(SIGINT, SignalFunc);
		signal(SIGTERM, SignalFunc);
#ifndef WIN32
		signal(SIGPIPE, SIG_IGN);
#endif
	}

}
