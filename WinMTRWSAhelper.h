#pragma once
#ifndef WINMTRWSAHELPER_H_
#define WINMTRWSAHELPER_H_

#include <WinSock2.h>


namespace winmtr::helper {
	class WSAHelper {
		bool valid;
	public:
		WSAHelper(WORD version)
		:valid(false){
			WSAData data = {};
			valid = !WSAStartup(version, &data);
		}

		~WSAHelper() noexcept {
			if (valid) {
				WSACleanup();
			}
		}

		explicit operator bool() const{
			return valid;
		}
	};
}

#endif