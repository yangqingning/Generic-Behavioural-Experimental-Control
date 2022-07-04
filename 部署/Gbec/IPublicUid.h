#pragma once
#include "UIDs.h"
class IPublicUid
{
public:
	virtual UIDs GetUID() const = 0;
};