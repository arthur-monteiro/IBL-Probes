#include "Debug.h"

void Wolf::Debug::sendError(std::string errorMessage)
{
	m_callback(Severity::ERROR, std::move(errorMessage));
}

void Wolf::Debug::sendWarning(std::string warningMessage)
{
	m_callback(Severity::WARNING, std::move(warningMessage));
}

void Wolf::Debug::sendInfo(std::string infoMessage)
{
	m_callback(Severity::INFO, std::move(infoMessage));
}

void Wolf::Debug::setCallback(std::function<void(Severity, std::string)> callback)
{
	m_callback = std::move(callback);
}
