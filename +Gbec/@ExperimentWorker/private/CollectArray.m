function Array=CollectArray(Serial)
import Gbec.UIDs
switch Serial.read(1,"uint8")
	case UIDs.Type_UIDs
		ArrayLength=UIDs(Serial.read(1,"uint8"));
	case UIDs.Type_bool
		ArrayLength=logical(Serial.read(1,"uint8"));
	case UIDs.Type_uint8_t
		ArrayLength=Serial.read(1,"uint8");
	case UIDs.Type_uint16_t
		ArrayLength=Serial.read(1,"uint16");
	case UIDs.Type_uint32_t
		ArrayLength=Serial.read(1,"uint32");
	otherwise
		warning("从串口返回了不正确的数据，信息收集中止");
		return;
end
switch Serial.read(1,"uint8")
	case UIDs.Type_UIDs
		Array=UIDs(Serial.read(ArrayLength,"uint8"));
	case UIDs.Type_bool
		Array=logical(Serial.read(ArrayLength,"uint8"));
	case UIDs.Type_uint8_t
		Array=Serial.read(ArrayLength,"uint8");
	case UIDs.Type_uint16_t
		Array=Serial.read(ArrayLength,"uint16");
	case UIDs.Type_uint32_t
		Array=Serial.read(ArrayLength,"uint32");
	case UIDs.StructStart
		Array=cell(ArrayLength,1);
		for a=1:ArrayLength
			Array{a}=CollectStruct(Serial);
		end
		try
			Array=[Array{:}];
		catch
		end
	case UIDs.ArrayStart
		Array=cell(ArrayLength,1);
		for a=1:ArrayLength
			Array{a}=CollectArray(Serial);
		end
	otherwise
		warning("从串口返回了不正确的数据，信息收集中止");
		return;
end