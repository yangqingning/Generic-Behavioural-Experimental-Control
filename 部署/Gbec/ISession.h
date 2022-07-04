#pragma once
#include "IInterruptablePublicUid.h"
class ISession : public IInterruptablePublicUid
{
public:
	virtual bool Continue(uint8_t NoDistinctUIDs, const UIDs *FinishedUID, const uint16_t *FinishedNumbers) const = 0;
};