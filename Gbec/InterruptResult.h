#pragma once
#include <Arduino.h>
enum InterruptResult : uint8_t
{
	//表示检查期间发生了暂停。不必手动处理此暂停，因为返回该值表示暂停已经结束了。你需要根据暂停的时间调整计时器。
	Paused,
	//表示检查期间没有发生中断，即没有收到指令要求暂停执行
	NotPaused,
	//表示收到指令要求放弃该过程。收到该返回值，应当立即进行资源清理，终结过程执行。
	Abort,
};