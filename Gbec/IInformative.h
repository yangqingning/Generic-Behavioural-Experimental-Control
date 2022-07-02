#pragma once
#include "SerialIO.h"
#include "UIDs.h"
class IInformative
{
public:
	virtual void WriteInformation() const = 0;
	static void WriteInformationStatic(const IInformative *II)
	{
		if (II)
			II->WriteInformation();
	}

protected:
	static void WriteType(UIDs Property)
	{
		SerialWrite(Type_UIDs);
	}
	static void WriteType(bool Property)
	{
		SerialWrite(Type_bool);
	}
	static void WriteType(uint8_t Property)
	{
		SerialWrite(Type_uint8_t);
	}
	static void WriteType(uint16_t Property)
	{
		SerialWrite(Type_uint16_t);
	}
	static void WriteType(uint32_t Property)
	{
		SerialWrite(Type_uint32_t);
	}
};
#define WriteProperty(PropertyName)       \
	SerialWrite(Property_##PropertyName); \
	WriteType(PropertyName);              \
	SerialWrite(PropertyName);
#define WriteComponent(ComponentName)                        \
	if (ComponentName)                                       \
	{                                                        \
		SerialWrite(Property_##ComponentName);               \
		SerialWrite(StructStart);                            \
		IInformative::WriteInformationStatic(ComponentName); \
		SerialWrite(StructEnd);                              \
	}
