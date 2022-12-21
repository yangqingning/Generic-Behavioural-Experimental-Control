#pragma once
template <typename Type>
inline void SerialWrite(const Type &Value)
{
  Serial.write((uint8_t *)&Value, sizeof(Type));
}
template <typename Type>
inline void SerialWrite(Type *Buffer, uint8_t Number)
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
inline void SerialRead(Type &Value)
{
  Serial.readBytes((uint8_t *)&Value, sizeof(Type));
}
template <typename Type>
inline void SerialRead(Type *Buffer, uint8_t Number)
{
  Serial.readBytes((uint8_t *)Buffer, sizeof(Type) * Number);
}