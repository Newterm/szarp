#include "baseconn.h"

void BaseConnection::OpenFinished()
{
	for (auto* listener : m_listeners) {
		listener->OpenFinished(this);
	}
}

void BaseConnection::ReadData(const std::vector<unsigned char>& data)
{
	for (auto* listener : m_listeners) {
		listener->ReadData(this, data);
	}
}

void BaseConnection::Error(short int event)
{
	for (auto* listener : m_listeners) {
		listener->ReadError(this, event);
	}
}

void BaseConnection::SetConfigurationFinished()
{
	for (auto* listener : m_listeners) {
		listener->SetConfigurationFinished(this);
	}
}
