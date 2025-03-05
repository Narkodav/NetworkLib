#pragma once
#include "../Namespaces.h"

namespace MultiThreading
{
	struct PointerControlBlock
	{
		size_t sharedCounter = 1;
		size_t weakCounter = 0;

		size_t objectSize = 0;			// Size of managed object
		bool isArray = false;			// Array handling flag
		char typeInfo;					// Type information
		void* userData;					// User-defined data
	};
}