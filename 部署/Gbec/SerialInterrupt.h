#pragma once
#include "ISession.h"
#include "UIDs.h"
#include "InterruptResult.h"
extern const ISession *CurrentSession;
void CheckerInstructions(UIDs Instruction, UIDs Default);
InterruptResult CheckInterrupt();