#pragma once
#include "UserComponents.h"
#include "Tagger.h"
#include "OneShotStimulator.h"
#include "SquareWaveStimulator.h"
#include "ToneStimulator.h"
#include "Monitor.h"
#include "CalmdownPhase.h"
#include "StimulatePhase.h"
#include "ResponsePhase.h"
#include "AwaitPhase.h"
#include "MonitorPhase.h"
#include "Trial.h"
#include "Session.h"
#define DefineTrial(NoPhases, UID, Name, ...) const Trial<NoPhases, UID> Name(new const IInterruptable *[NoPhases] { __VA_ARGS__ });
#define TrialList(NoTrials, ...) new const IInterruptablePublicUid *[NoTrials] { __VA_ARGS__ }
#define NumberList(NoNumbers, ...) \
	new const uint16_t[NoNumbers] { __VA_ARGS__ }