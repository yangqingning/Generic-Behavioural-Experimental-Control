#pragma once
#include "IInterruptable.h"
#include "SerialInterrupt.h"
#include "MonitorBase.h"
#include <TimersOneForAll.h>
template <uint8_t TimerCode, uint16_t Milliseconds, bool ReportOnce, UIDs UID>
class MonitorPhase : public IInterruptable
{
    const MonitorBase *const Monitor;

public:
    MonitorPhase(const MonitorBase *Monitor) : Monitor(Monitor) {}
    bool TryRun() const override
    {
        uint32_t MillisecondsElapsed;
        if (ReportOnce)
        {
            switch (Monitor->StartMonitor(Milliseconds, CheckInterrupt, &MillisecondsElapsed))
            {
            case Detected:
                SerialWrite(Signal_Detected);
                SerialWrite(Monitor->GetUID());
                if (Milliseconds > MillisecondsElapsed)
                    TimersOneForAll::Delay<TimerCode>(Milliseconds - MillisecondsElapsed);
                return false;
            case TimeUp:
                SerialWrite(Signal_TimeUp);
                SerialWrite(Monitor->GetUID());
                return false;
            default:
                return true;
            }
        }
        else
        {
            int32_t MillisecondsLeft = Milliseconds;
            while (MillisecondsLeft > 0)
                switch (Monitor->StartMonitor(MillisecondsLeft, CheckInterrupt, &MillisecondsElapsed))
                {
                case Detected:
                    SerialWrite(Signal_Detected);
                    SerialWrite(Monitor->GetUID());
                    MillisecondsLeft -= MillisecondsElapsed;
                    break;
                case TimeUp:
                    SerialWrite(Signal_TimeUp);
                    SerialWrite(Monitor->GetUID());
                    MillisecondsLeft = 0;
                    break;
                default:
                    return true;
                }
            SerialWrite(Signal_TimeUp);
            SerialWrite(Monitor->GetUID());
            return false;
        }
    }
    void WriteInformation() const override
    {
        WriteProperty(UID);
        WriteComponent(Monitor);
        WriteProperty(Milliseconds);
        WriteProperty(ReportOnce);
    }
};