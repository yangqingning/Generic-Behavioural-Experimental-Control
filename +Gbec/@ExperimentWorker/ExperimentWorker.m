classdef ExperimentWorker<handle
    %实验主控制器
    %一般不直接使用该类，而是通过*_Client.mlx实时脚本作为用户界面来操纵实验。当然您也可以根据我们提供的Client脚本学习本类的使用方法。
	properties
		%实验记录保存路径
		SavePath(1,1)string
		%断线重连尝试间隔秒数
		RetryInterval(1,1)double=2
		%断线重连尝试次数
		MaxRetryTimes(1,1)uint8=3
		%实验结束后是否保存记录
		SaveFile(1,1)logical
		%喵提醒码
		EndMiaoCode(1,1)string=""
		%喵提醒重试次数
		HttpRetryTimes(1,1)uint8=3
		%会话结束后是否自动关闭串口
		ShutDownSerialAfterSession(1,1)logical
		%当前运行会话
		Session(1,1)Gbec.UID
		%视频输入设备
		VideoInput
		%日期时间
		DateTime
		%鼠名
		Mouse string
		State=Gbec.UID.State_SessionInvalid
	end
	properties(Access=private)
		Serial internal.Serialport
	end
	properties(GetAccess=private,SetAccess=immutable)
		WatchDog(1,1)timer=timer(StartDelay=10,TimerFcn=@obj.CloseSerial);
		EventRecorder(1,1)MATLAB.EventLogger
		TrialRecorder(1,1)MATLAB.EventLogger
	end
	properties(Dependent)
		%如果启用会话结束后自动关闭串口功能，该属性设置关闭串口的延迟时间
		SerialFreeTime(1,1)double
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
		function Signal=WaitForSignal(EW)
			EW.Serial.Timeout=1;
			while true
				Signal=EW.Serial.read(1,'uint8');
				if isempty(Signal)
					warning("Arduino设备在%us内未响应，请检查连接，或继续等待程序重试",EW.Serial.Timeout);
					EW.Serial.Timeout=EW.Serial.Timeout*2;
				else
					break;
				end
			end
		end
		function RestoreSession(EW)
			TrialsDone=EW.TrialRecorder.GetTimeTable().Event;
			DistinctTrials=unique(TrialsDone);
			NumTrials=countcats(categorical(TrialsDone));
			EW.Serial.write(Gbec.UID.API_Restore,"uint8");
			EW.Serial.write(EW.Session,"uint8");
			for T=1:numel(DistinctTrials)
				EW.Serial.write(DistinctTrials(T),'uint8');
				EW.Serial.write(NumTrials(T),'uint16');
			end
			if EW.WaitForSignal~=UID.Signal_SessionRestored
				GbecException.Unexpected_response_from_Arduino.Throw;
			end
		end
		function AbortAndSave(EW)
			EW.EventRecorder.LogEvent(UID.State_SessionAborted);
			disp('会话已放弃');
			if ~isempty(EW.VideoInput)
				stop(EW.VideoInput);
			end
			if EW.SaveFile
				if input("实验已放弃，是否保存现有数据？y/n","s")~="n"
					EW.SaveInformation(EW.Session);
				else
					delete(EW.SavePath);
				end
			else
				warning("数据未保存");
			end
			EW.WatchDog.start;
			EW.State=UID.State_SessionAborted;
		end
		RunningCallback(EW,~,~)
		InterruptRetry(EW,~,~)
	end
	methods
		function obj=ExperimentWorker
			obj.WatchDog=timer(StartDelay=10,TimerFcn=@obj.CloseSerial);
			obj.EventRecorder=Gbec.internal.TimeRecorder;
			obj.TrialRecorder=MATLAB.DataTypes.ArrayBuilder;
            disp(['通用行为实验控制器v' Gbec.Version().Me ' by 张天夫']);
		end
		function SerialInitialize(EW,SerialPort)
			%初始化串口
			%输入参数：SerialPort(1,1)string，串口名称
			EW.Serial.configureCallback("off");
			try
				assert(EW.Serial.Port==SerialPort);
				EW.Serial.write(Gbec.UID.API_IsReady,"uint8");
				assert(EW.WaitForSignal==Gbec.UID.Signal_SerialReady);
			catch
				delete(EW.Serial);
				EW.Serial=serialport(SerialPort,9600);
				%刚刚初始化时不能向串口发送数据，只能等待Arduino主动宣布初始化完毕
				if EW.WaitForSignal~=Gbec.UID.Signal_SerialReady
					Gbec.GbecException.Serial_handshake_failed.Throw;
				end
			end
			EW.WatchDog.stop;
			EW.Serial.ErrorOccurredFcn=@EW.InterruptRetry;
		end
		function StartTest(EW,TestUID,TestTimes)
			arguments
				EW
				TestUID
				TestTimes=1
			end
			%开始检查监视器
			%输入参数：DeviceUID(1,1)Gbec.UID，设备标识符
			import Gbec.UID
			EW.WatchDog.stop;
			EW.Serial.write(UID.API_TestStart,"uint8");
			EW.Serial.write(TestUID,'uint8');
			EW.Serial.write(TestTimes,'uint16');
			switch EW.WaitForSignal
				case UID.Signal_NoSuchTest
					Gbec.GbecException.Test_not_found_in_Arduino.Throw;
				case UID.TestStartedAutoStop
					disp('测试开始（自动结束）');
				case UID.TestStartedManualStop
					disp('测试开始（手动结束）');
					EW.Serial.configureCallback('byte',1,@EW.MonitorCallback);
				case UID.State_SessionRunning
					Gbec.GbecException.Cannot_test_while_session_running.Throw;
				otherwise
					Gbec.GbecException.Unexpected_response_from_Arduino.Throw;
			end
		end
		function StopTest(EW,TestUID)
			arguments
				EW
				TestUID=Gbec.UID.Test_Last
			end
			%停止检查监视器
			import Gbec.UID
			EW.Serial.write(UID.API_TestStop,"uint8");
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
			%检查刺激器，按一次回车给一个刺激，输入任意字符停止检查
			%输入参数：
			%DeviceUID(1,1)Gbec.UID，设备标识符
			%EnterPrompt(1,1)string，提示文字，将显示在命令行中
			while input(EnterPrompt,"s")==""
				EW.StartTest(TestUID,1);
			end
		end
		function StartSession(EW)
			%开始会话
			import Gbec.UID
			import Gbec.GbecException
			if EW.State==UID.State_SessionRestored
				GbecException.There_is_already_a_session_being_paused.Throw;
			end
			EW.Serial.write(UID.API_Start,"uint8");
			EW.Serial.write(EW.Session,"uint8");
			switch EW.WaitForSignal
				case UID.State_SessionRunning
					GbecException.There_is_already_a_session_running.Throw;
				case UID.State_SessionPaused
					GbecException.There_is_already_a_session_being_paused.Throw;
				case UID.Signal_NoSuchSession
					GbecException.Session_not_found_on_Arduino.Throw;
				case UID.Signal_SessionStarted
					obj.EventRecorder.Reset;
					obj.TrialRecorder.Reset;
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
		function PauseSession(EW)
			%暂停会话
			import Gbec.UID
			import Gbec.GbecException
			if EW.State==UID.State_SessionRestored||EW.State==UID.State_SessionPaused
				GbecException.Cannot_pause_a_paused_session.Throw;
			end
			EW.Serial.write(UID.API_Pause,"uint8");
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
				EW.Serial.configureCallback('byte',1,@EW.RunningCallback);
				EW.State=UID.State_SessionRunning;
			else
				EW.Serial.write(UID.API_Continue,"uint8");
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
		function AbortSession(EW)
			%放弃会话
			import Gbec.UID
			import Gbec.GbecException
			switch EW.State
				case UID.State_SessionRestored
					EW.AbortAndSave;
				case UID.State_SessionAborted
					GbecException.Cannot_abort_an_aborted_session.Throw;
				otherwise
					EW.Serial.write(UID.API_Abort,"uint8");
					switch EW.WaitForSignal
						case UID.State_SessionInvalid
							GbecException.No_sessions_are_running.Throw;
						case UID.State_SessionAborted
							EW.Serial.configureCallback('off');
							EW.AbortAndSave;
						case UID.State_SessionFinished
							GbecException.Cannot_abort_a_finished_session.Throw;
						otherwise
							GbecException.Unexpected_response_from_Arduino.Throw;
					end
			end
		end
		function delete(EW)
			EW.WatchDog.stop;
			delete(EW.Serial);
		end
		function CloseSerial(EW,~,~)
			%关闭串口
			if EW.ShutDownSerialAfterSession
				delete(EW);
				disp("串口已关闭");
			end
		end
		function SFT=get.SerialFreeTime(EW)
			SFT=EW.WatchDog.StartDelay;
		end
		function set.SerialFreeTime(EW,SFT)
			EW.WatchDog.StartDelay=SFT;
		end
		function Information = GetInformation(EW,SessionUID)
			arguments
				EW
				SessionUID=Gbec.UID.Session_Current
			end
			%获取会话信息
			%返回值：Information(1,1)struct，信息结构体
			import Gbec.UID
			EW.WatchDog.stop;
			EW.Serial.write([UID.API_GetInfo,SessionUID],"uint8");
			switch EW.WaitForSignal
				case UID.Signal_SessionInvalid
					Gbec.GbecException.Must_run_session_before_getting_information.Throw;
				case UID.Signal_InfoStart
					Information=CollectStruct(EW.Serial);
				otherwise
					Gbec.GbecException.Unexpected_response_from_Arduino.Throw;
			end
		end
		function SaveInformation(EW,SessionUID)
			arguments
				EW
				SessionUID=Gbec.UID.Session_Current
			end
			%获取并保存会话信息
			DateTimes=table;
			EW.DateTime.Second=0;
			DateTimes.DateTime=EW.DateTime;
			DateTimes.Mouse=EW.Mouse;
			DateTimes.Metadata={EW.GetInformation(SessionUID)};
			Design=char(EW.Session);
			Blocks=table;
			Blocks.DateTime=EW.DateTime;
			Blocks.Design=string(Design(9:end));
			TimeTable=EW.EventRecorder.GetTimeTable;
			TimeTable.Event=string(TimeTable.Event);
			Blocks.EventLog={TimeTable};
			Blocks.BlockIndex=0x1;
			Blocks.BlockUID=0x001;
			Trials=table;
			Stimulus=EW.TrialRecorder.GetTimeTable;
			NumTrials=height(Stimulus);
			TrialIndex=(0x001:NumTrials)';
			Trials.TrialUID=TrialIndex;
			Trials.BlockUID(:)=0x001;
			Trials.TrialIndex=TrialIndex;
			Trials.Stimulus=string(Stimulus.Event);
			Trials.Time=Stimulus.Time;
			Version=Gbec.Version;
			save(EW.SavePath,'DateTimes','Blocks','Trials','Version');
			SaveDirectory=fileparts(EW.SavePath);
			disp("数据已保存到"+"<a href=""matlab:winopen('"+EW.SavePath+"');"">"+EW.SavePath+"</a> <a href=""matlab:cd('"+SaveDirectory+"');"">切换当前文件夹</a> <a href=""matlab:winopen('"+SaveDirectory+"');"">打开数据文件夹</a>");
		end
	end
end