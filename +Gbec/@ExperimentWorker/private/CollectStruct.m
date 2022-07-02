function Struct=CollectStruct(Serial)
import Gbec.UIDs
while true
	Signal=char(UIDs(Serial.read(1,"uint8")));
	if startsWith(Signal,"Property_")
		Field=Signal(strlength("Property_")+1:end);
	else
		switch Signal
			case UIDs.Type_UIDs
				Struct.(Field)=UIDs(Serial.read(1,"uint8"));
			case UIDs.Type_bool
				Struct.(Field)=logical(Serial.read(1,"uint8"));
			case UIDs.Type_uint8_t
				Struct.(Field)=Serial.read(1,"uint8");
			case UIDs.Type_uint16_t
				Struct.(Field)=Serial.read(1,"uint16");
			case UIDs.Type_uint32_t
				Struct.(Field)=Serial.read(1,"uint32");
			case UIDs.StructStart
				Struct.(Field)=CollectStruct(Serial);
			case UIDs.ArrayStart
				Struct.(Field)=CollectArray(Serial);
			case UIDs.StructEnd
				break;
			otherwise
				warning("从串口返回了不正确的数据，信息收集中止");
				break;
		end
	end
end
end