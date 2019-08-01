#ifndef ARGUMENT_H_
#define ARGUMENT_H_

#include "storage.h"

namespace bumo {
	class Argument {
	public:
		Argument();
		~Argument();
	};

	extern bool g_enable_;
	extern bool g_ready_;

	void InstallSignal();
}
#endif