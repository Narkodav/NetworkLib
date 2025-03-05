#pragma once
#include "../Namespaces.h"

namespace MultiThreading
{
	template<typename E, size_t NumEvents>
	struct EventPolicy {
		using Enum = E;
		static constexpr size_t EVENT_NUM = NumEvents;

		template<E Event>
		struct Traits;  // Base traits template
	};
}


