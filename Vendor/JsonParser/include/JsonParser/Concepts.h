#pragma once

namespace Json
{
	template<typename T>
	concept Stream = requires(T & t, char c) {
		t.get(c);
		t.eof();
	};

	template<typename T>
	concept Container = requires(T & t) {
		t.size();
		t[0];
	};
}