function StartSession(EW)
%开始会话
import Gbec.UID
import Gbec.GbecException
if EW.State==UID.State_SessionRestored
	GbecException.There_is_already_a_session_being_paused.Throw;
end
if input("将保存为："+replace(EW.SavePath,"\","\\")+"，确认？y/n","s")=="y"
	if isfile(EW.SavePath)&&input("文件已存在，是否覆盖？y/n","s")~="y"
		disp("将不会自动写入文件");
		EW.SaveFile=false;
	else
		Fid=fopen(EW.SavePath,'w');
		if Fid==-1
			mkdir(fileparts(EW.SavePath));
			Fid=fopen(EW.SavePath,'w');
		end
		fclose(Fid);
		EW.SaveFile=true;
	end
else
	disp("取消会话");
	return;
end
EW.ApiCall(UID.API_Start);
EW.Serial.write(EW.SessionUID,"uint8");
switch EW.WaitForSignal
	case UID.State_SessionRunning
		EW.Serial.configureCallback('byte',1,@EW.RunningCallback);
		GbecException.There_is_already_a_session_running.Throw;
	case UID.State_SessionPaused
		GbecException.There_is_already_a_session_being_paused.Throw;
	case UID.Signal_NoSuchSession
		GbecException.Session_not_found_on_Arduino.Throw;
	case UID.Signal_SessionStarted
		EW.WatchDog.stop;
		EW.EventRecorder.Reset;
		EW.TrialRecorder.Reset;
		EW.Serial.configureCallback("byte",1,@EW.RunningCallback);
		if ~isempty(EW.VideoInput)
			preview(EW.VideoInput);
			start(EW.VideoInput);
			waitfor(EW.VideoInput,'Running','on');
		end
		EW.State=UID.State_SessionRunning;
		disp('会话开始');
	otherwise
		GbecException.Unexpected_response_from_Arduino.Throw;
end
end