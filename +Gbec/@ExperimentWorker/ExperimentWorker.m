classdef ExperimentWorker<handle
    %实验主控制器
    %一般不直接使用该类，而是通过*_Client.mlx实时脚本作为用户界面来操纵实验。当然您也可以根据我们提供的Client脚本学习本类的使用方法。
	properties
		%实验记录保存路径
		SavePath(1,1)string
		%实验结束后是否保存记录
		SaveFile(1,1)logical
		%喵提醒码
		EndMiaoCode(1,1)string=""
		%喵提醒重试次数
		HttpRetryTimes(1,1)uint8=3
		%会话结束后是否自动关闭串口
		ShutDownSerialAfterSession(1,1)logical
		%当前运行会话
		SessionUID(1,1)Gbec.UID
		%日期时间
		DateTime
		%鼠名
		Mouse string
		%断线重连尝试间隔秒数
		RetryInterval(1,1)double=2
		%断线重连尝试次数
		MaxRetryTimes(1,1)uint8=3
		%视频对象 
		VideoInput
	end
	properties(Access=private)
		Serial internal.Serialport
		State=Gbec.UID.State_SessionInvalid		
		%没有对象无法初始化
		WatchDog
	end
	properties(GetAccess=private,SetAccess=immutable)
		EventRecorder(1,1)MATLAB.EventLogger
		TrialRecorder(1,1)MATLAB.EventLogger
	end
	properties(Dependent)
		%如果启用会话结束后自动关闭串口功能，该属性设置关闭串口的延迟时间
		SerialFreeTime(1,1)double
	end
	methods(Static,Access=private)
		function ReleaseSerial(Serial)
			delete(Serial);
			disp('串口已释放');
		end
	end
	methods(Access=private)
		function MonitorCallback(EW,~,~)
			persistent SignalIndex
			if isempty(SignalIndex)
				SignalIndex=0;
			end
			while EW.Serial.NumBytesAvailable
				if EW.Serial.read(1,"uint8")==Gbec.UID.Signal_MonitorHit
					SignalIndex=SignalIndex+1;
					disp("成功检测到触摸信号："+num2str(SignalIndex));
				else
					warning("收到非法信号");
				end
			end
		end
		function AbortAndSave(EW)
			EW.EventRecorder.LogEvent(Gbec.UID.State_SessionAborted);
			disp('会话已放弃');
			if ~isempty(EW.VideoInput)
				stop(EW.VideoInput);
			end
			if EW.SaveFile
				if input("实验已放弃，是否保存现有数据？y/n","s")~="n"
					EW.SaveInformation(EW.SessionUID);
				else
					delete(EW.SavePath);
				end
			else
				warning("数据未保存");
			end
			EW.State=Gbec.UID.State_SessionAborted;
			EW.WatchDog.start;
		end
		function Signal=WaitForSignal(obj)
			obj.Serial.Timeout=1;
			while true
				Signal=obj.Serial.read(1,'uint8');
				if isempty(Signal)
					warning("Arduino设备在%us内未响应，请检查连接，或继续等待程序重试",obj.Serial.Timeout);
					obj.Serial.Timeout=obj.Serial.Timeout*2;
				else
					break;
				end
			end
		end
		function ApiCall(obj,ApiUid)
			obj.Serial.flush;
			obj.Serial.write(ApiUid,'uint8');
			switch obj.WaitForSignal
				case Gbec.UID.Signal_ApiFound
				case Gbec.UID.Signal_ApiInvalid
					Gbec.GbecException.Arduino_received_unsupported_API_code.Throw;
				otherwise
					Gbec.GbecException.Unexpected_response_from_Arduino.Throw;
			end
		end
		function RestoreSession(obj)
			TrialsDone=obj.TrialRecorder.GetTimeTable().Event;
			DistinctTrials=unique(TrialsDone);
			NumTrials=countcats(categorical(TrialsDone));
			obj.ApiCall(Gbec.UID.API_Restore);
			obj.Serial.write(obj.SessionUID,"uint8");
			NDT=numel(DistinctTrials);
			obj.Serial.write(NDT,'uint8');
			for T=1:NDT
				obj.Serial.write(DistinctTrials(T),'uint8');
				obj.Serial.write(NumTrials(T),'uint16');
			end
			if obj.WaitForSignal==Gbec.UID.Signal_SessionRestored
				obj.Serial.configureCallback("byte",1,@obj.RunningCallback);
			else
				Gbec.GbecException.Unexpected_response_from_Arduino.Throw;
			end
		end
	end
	methods
		function obj=ExperimentWorker
			%构造对象，建议使用MATLAB.Lang.Owner包装对象，不要直接存入工作区，否则清空变量时可能不能正确断开串口
            disp(['通用行为实验控制器v' Gbec.Version().Me ' by 张天夫']);
			obj.WatchDog=timer(StartDelay=10,TimerFcn=@(~,~)Gbec.ExperimentWorker.ReleaseSerial(obj.Serial));
		end
		function SerialInitialize(obj,SerialPort)
			%初始化串口
			%# 语法
			% ```MATLAB
			% obj.SerialInitialize(SerialPort);
			% ```
			%# 输入参数
			% SerialPort(1,1)string，串口名称
			try
				assert(obj.Serial.Port==SerialPort);
				obj.ApiCall(Gbec.UID.API_IsReady);
				assert(obj.WaitForSignal==Gbec.UID.Signal_SerialReady);
			catch
				delete(obj.Serial);
				obj.Serial=serialport(SerialPort,9600);
				%刚刚初始化时不能向串口发送数据，只能等待Arduino主动宣布初始化完毕
				if obj.WaitForSignal~=Gbec.UID.Signal_SerialReady
					Gbec.GbecException.Serial_handshake_failed.Throw;
				end
				obj.WatchDog.TimerFcn=@(~,~)Gbec.ExperimentWorker.ReleaseSerial(obj.Serial);
				obj.Serial.ErrorOccurredFcn=@obj.InterruptRetry;
			end
			obj.WatchDog.stop;
			obj.Serial.configureCallback("off");
		end
		function StartTest(EW,TestUID,TestTimes)
			%开始运行测试
			%测试分为自动结束和手动结束两种类型。自动结束类型可以指定测试次数，默认1次；手动结束类型则无所谓测试次数，需要使用StopTest才能停止。
			%# 语法
			% ```MATLAB
			% obj.StartTest(TestUID);
			% %开始具有指定UID的测试
			%
			% obj.StartTest(TestUID,TestTimes);
			% %额外指定测试次数
			% ```
			%# 输入参数
			% TestUID(1,1)Gbec.UID，要开始的测试UID
			% TestTimes(1,1)=1，测试次数。如果是手动结束类测试，该参数将被忽略
			arguments
				EW
				TestUID
				TestTimes=1
			end
			import Gbec.UID
			EW.WatchDog.stop;
			EW.ApiCall(UID.API_TestStart);
			EW.Serial.write(TestUID,'uint8');
			EW.Serial.write(TestTimes,'uint16');
			switch EW.WaitForSignal
				case UID.Signal_NoSuchTest
					Gbec.GbecException.Test_not_found_in_Arduino.Throw;
				case UID.Signal_TestStartedAutoStop
					disp('测试开始（自动结束）');
				case UID.Signal_TestStartedManualStop
					disp('测试开始（手动结束）');
					EW.Serial.configureCallback('byte',1,@EW.MonitorCallback);
				case UID.State_SessionRunning
					Gbec.GbecException.Cannot_test_while_session_running.Throw;
				otherwise
					Gbec.GbecException.Unexpected_response_from_Arduino.Throw;
			end
		end
		function StopTest(EW,TestUID)
			%停止测试
			%仅适用于手动结束类测试。自动结束类测试不能停止。
			%# 语法
			% ```MATLAB
			% obj.StopTest(TestUID);
			% ```
			%# 输入参数
			% TestUID(1,1)Gbec.UID=Gbec.UID.Test_Last，要停止的测试UID，只能选择手动结束类测试进行停止。默认停止上一个开始的测试。
			arguments
				EW
				TestUID=Gbec.UID.Test_Last
			end
			import Gbec.UID
			EW.ApiCall(UID.API_TestStop);
			EW.Serial.write(TestUID,'uint8');
			switch EW.WaitForSignal
				case UID.Signal_TestStopped
					disp('测试结束');
					EW.Serial.configureCallback('off');
				case UID.State_SessionRunning
					disp('测试结束');
				case UID.Signal_NoLastTest
					Gbec.GbecException.Last_test_not_running_or_unstoppable.Throw;
				case UID.Signal_NoSuchTest
					Gbec.GbecException.Test_not_found_on_Arduino.Throw;
				otherwise
					Gbec.GbecException.Unexpected_response_from_Arduino.Throw;
			end
		end
		function OneEnterOneCheck(EW,TestUID,EnterPrompt)
			%对于自动结束类测试，提供一个"按一次回车测试一次"的友好交互
			%调用此方法后，会在命令行窗口显示提示，等待用户输入，用户按回车则运行一次测试，输入任意字符后按回车则停止测试。详见SelfCheck_Client.mlx中的示例用法。手动
			% 结束类测试不支持此方法。
			%# 语法
			% ```MATLAB
			% obj.OneEnterOneCheck(TestUID,EnterPrompt);
			% ```
			%# 输入参数
			%TestUID(1,1)Gbec.UID，要运行的测试UID，必须是自动结束类测试
			%EnterPrompt(1,1)string，提示文字，将显示在命令行中
			import Gbec.UID
			import Gbec.GbecException
			EW.WatchDog.stop;
			while input(EnterPrompt,"s")==""
				EW.ApiCall(UID.API_TestStart);
				EW.Serial.write(TestUID,'uint8');
				EW.Serial.write(1,'uint16');
				switch EW.WaitForSignal
					case UID.Signal_NoSuchTest
						GbecException.Test_not_found_in_Arduino.Throw;
					case UID.Signal_TestStartedAutoStop
					case UID.Signal_TestStartedManualStop
						EW.ApiCall(UID.API_TestStop);
						EW.Serial.write(TestUID,'uint8');
						if EW.WaitForSignal~=UID.Signal_TestStopped
							GbecException.Unexpected_response_from_Arduino.Throw;
						else
							GbecException.Cannot_OneEnterOneCheck_on_manual_stopped_test.Throw;
						end
					case UID.State_SessionRunning
						GbecException.Cannot_test_while_session_running.Throw;
					otherwise
						GbecException.Unexpected_response_from_Arduino.Throw;
				end
			end
		end
		function PauseSession(EW)
			%暂停会话
			import Gbec.UID
			import Gbec.GbecException
			if EW.State==UID.State_SessionRestored||EW.State==UID.State_SessionPaused
				GbecException.Cannot_pause_a_paused_session.Throw;
			end
			EW.ApiCall(UID.API_Pause);
			switch EW.WaitForSignal
				case UID.State_SessionInvalid
					GbecException.No_sessions_are_running.Throw;
				case UID.State_SessionPaused
					EW.EventRecorder.LogEvent(UID.State_SessionPaused);
					disp('会话暂停');
					EW.Serial.configureCallback('off');
					EW.State=UID.State_SessionPaused;
				case UID.State_SessionAborted
					GbecException.Cannot_pause_an_aborted_session.Throw;
				case UID.State_SessionFinished
					GbecException.Cannot_pause_a_finished_session.Throw;
				otherwise
					GbecException.Unexpected_response_from_Arduino.Throw;
			end
		end
		function ContinueSession(EW)
			%继续会话
			import Gbec.UID
			import Gbec.GbecException
			if EW.State==UID.State_SessionRestored
				EW.RestoreSession;
				EW.EventRecorder.LogEvent(UID.Signal_SessionContinue);
				disp('会话继续');
				EW.State=UID.State_SessionRunning;
			else
				EW.ApiCall(UID.API_Continue);
				switch EW.WaitForSignal
					case UID.State_SessionInvalid
						GbecException.No_sessions_are_running.Throw;
					case UID.Signal_SessionContinue
						EW.EventRecorder.LogEvent(UID.Signal_SessionContinue);
						disp('会话继续');
						EW.Serial.configureCallback('byte',1,@EW.RunningCallback);
						EW.State=UID.State_SessionRunning;
					case UID.State_SessionAborted
						GbecException.Cannot_continue_an_aborted_session.Throw;
					case UID.State_SessionFinished
						GbecException.Cannot_continue_a_finished_session.Throw;
					case UID.State_SessionRunning
						GbecException.Cannot_continue_a_running_session.Throw;
					otherwise
						GbecException.Unexpected_response_from_Arduino.Throw;
				end
			end
		end
		function AbortSession(obj)
			%放弃会话
			import Gbec.UID
			import Gbec.GbecException
			switch obj.State
				case UID.State_SessionRestored
					obj.AbortAndSave;
				case UID.State_SessionAborted
					GbecException.Cannot_abort_an_aborted_session.Throw;
				otherwise
					obj.ApiCall(UID.API_Abort);
					switch obj.WaitForSignal
						case UID.State_SessionInvalid
							GbecException.No_sessions_are_running.Throw;
						case UID.State_SessionAborted
							obj.Serial.configureCallback('off');
							obj.AbortAndSave;%这个函数调用包含了启用看门狗
						case UID.State_SessionFinished
							GbecException.Cannot_abort_a_finished_session.Throw;
						otherwise
							GbecException.Unexpected_response_from_Arduino.Throw;
					end
			end
		end
		function delete(obj)
			delete(obj.WatchDog);
			delete(obj.Serial);
		end
		function SFT=get.SerialFreeTime(EW)
			SFT=EW.WatchDog.StartDelay;
		end
		function set.SerialFreeTime(EW,SFT)
			EW.WatchDog.StartDelay=SFT;
		end
		function Information = GetInformation(EW,SessionUID)
			%获取会话信息
			%# 语法
			% ```MATLAB
			% Info=obj.GetInformation;
			% %获取上次运行的会话信息
			%
			% Info=obj.GetInformation(SessionUID);
			% %获取指定UID的会话信息
			% ```
			%# 输入参数
			% SessionUID(1,1)Gbec.UID=Gbec.UID.Session_Current，要获取的会话UID，默认获取之前运行的会话信息
			%# 返回值
			% Information(1,1)struct，信息结构体
			arguments
				EW
				SessionUID=Gbec.UID.Session_Current
			end
			import Gbec.UID
			EW.WatchDog.stop;
			EW.ApiCall(UID.API_GetInfo);
			EW.Serial.write(SessionUID,'uint8');
			switch EW.WaitForSignal
				case UID.State_SessionInvalid
					Gbec.GbecException.Must_run_session_before_getting_information.Throw;
				case UID.Signal_InfoStart
					Information=CollectStruct(EW.Serial);
				case UID.State_SessionRunning
					Gbec.GbecException.Cannot_get_information_while_session_running.Throw;
				otherwise
					Gbec.GbecException.Unexpected_response_from_Arduino.Throw;
			end
		end
		function SaveInformation(obj)
			%获取并保存上次运行的会话信息到SaveFile文件。
			DateTimes=table;
			obj.DateTime.Second=0;
			DateTimes.DateTime=obj.DateTime;
			DateTimes.Mouse=obj.Mouse;
			DateTimes.Metadata={obj.GetInformation(obj.SessionUID)};
			Design=char(obj.SessionUID);
			Blocks=table;
			Blocks.DateTime=obj.DateTime;
			Blocks.Design=string(Design(9:end));
			EventLog=obj.EventRecorder.GetTimeTable;
			EventLog.Event=Gbec.LogTranslate(EventLog.Event);
			Blocks.EventLog={EventLog};
			Blocks.BlockIndex=0x1;
			Blocks.BlockUID=0x001;
			Trials=table;
			Stimulus=obj.TrialRecorder.GetTimeTable;
			NumTrials=height(Stimulus);
			TrialIndex=(0x001:NumTrials)';
			Trials.TrialUID=TrialIndex;
			Trials.BlockUID(:)=0x001;
			Trials.TrialIndex=TrialIndex;
			Trials.Stimulus=Gbec.LogTranslate(Stimulus.Event);
			Trials.Time=Stimulus.Time;
			Version=Gbec.Version;
			save(obj.SavePath,'DateTimes','Blocks','Trials','Version');
			SaveDirectory=fileparts(obj.SavePath);
			disp("数据已保存到"+"<a href=""matlab:winopen('"+obj.SavePath+"');"">"+obj.SavePath+"</a> <a href=""matlab:cd('"+SaveDirectory+"');"">切换当前文件夹</a> <a href=""matlab:winopen('"+SaveDirectory+"');"">打开数据文件夹</a>");
		end
		function PeekState(EW)
			%观察会话当前的运行状态
			EW.ApiCall(Gbec.UID.API_Peek);
			disp(Gbec.UID(EW.WaitForSignal));
		end
	end
end