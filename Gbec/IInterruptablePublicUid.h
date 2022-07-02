#pragma once
#include "IInterruptable.h"
#include "IPublicUid.h"
class IInterruptablePublicUid : public IInterruptable, public IPublicUid
{
};