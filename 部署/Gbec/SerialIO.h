#pragma once
#include <Arduino.h>
template <typename Type, Type Value>
void SerialWrite()
{
	Type V = Value;
	Serial.write((uint8_t *)&V, sizeof(Type));
}
template <typename Type>
void SerialWrite(Type Value)
{
	Serial.write((uint8_t *)&Value, sizeof(Type));
}
template <typename Type>
void SerialWrite(Type *Buffer, uint8_t Number)
{
	Serial.write((uint8_t *)Buffer, sizeof(Type) * Number);
}
template <typename Type>
Type SerialRead()
{
	Type Buffer;
	Serial.readBytes((uint8_t *)&Buffer, sizeof(Type));
	return Buffer;
}
template <typename Type>
Type SerialRead(Type &Value)
{
	Serial.readBytes((uint8_t *)&Value, sizeof(Type));
	return Value;
}
template <typename Type>
Type *SerialRead(Type *Buffer, uint8_t Number)
{
	Serial.readBytes((uint8_t *)Buffer, sizeof(Type) * Number);
	return Buffer;
}