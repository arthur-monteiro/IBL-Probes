#pragma once

#include <string>
#include <functional>
#include <utility>

namespace Wolf
{	
	class Debug
	{
	public:
		enum class Severity { ERROR, WARNING, INFO };

		static void sendError(std::string errorMessage);
		static void sendWarning(std::string warningMessage);
		static void sendInfo(std::string infoMessage);

		static void setCallback(std::function<void(Severity, std::string)> callback);

	private:
		Debug() {};
		~Debug() {}
	};

	static std::function<void(Debug::Severity, std::string)> m_callback;

}
