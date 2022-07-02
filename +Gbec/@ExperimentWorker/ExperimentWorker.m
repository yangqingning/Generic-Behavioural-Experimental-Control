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
		Session(1,1)Gbec.UIDs
		%视频输入设备
		VideoInput
		%日期时间
		DateTime
		%鼠名
		Mouse
	end
	properties(Access=private)
		Serial internal.Serialport
		SessionRunning(1,1)logical=false;
		CallbackTemp function_handle
		CurrentTrial(1,:)char
	end
	properties(GetAccess=private,SetAccess=immutable)
		WatchDog(1,1)timer=timer(StartDelay=10,TimerFcn=@obj.CloseSerial);
		Recorder Gbec.internal.TimeRecorder
		TrialRecorder MATLAB.DataTypes.ArrayBuilder
	end
	properties(Dependent)
		%如果启用会话结束后自动关闭串口功能，该属性设置关闭串口的延迟时间
		SerialFreeTime(1,1)double
	end
	methods(Access=private)
		function HandleEvent(EW,Event)
			Event=Gbec.LogTranslate(Event);
			EW.Recorder.Hit(Event);
			fprintf(Event+" ");
		end
		function FindDevice(EW,DeviceUID)
			import Gbec.UIDs
			EW.Serial.write(DeviceUID,"uint8");
			switch EW.Serial.read(1,"uint8")
				case UIDs.Signal_DeviceFound
					return;
				case UIDs.Signal_DeviceNotFound
					error("设备UID不存在");
				otherwise
					error("从串口传来了不正确数据，程序中止");
			end
		end
		function MonitorCallback(EW,~,~)
			persistent SignalIndex
			if isempty(SignalIndex)
				SignalIndex=0;
			end
			if EW.Serial.NumBytesAvailable>0
				switch EW.Serial.read(1,"uint8")
					case Gbec.UIDs.Signal_Detected
						SignalIndex=SignalIndex+1;
						disp("成功检测到触摸信号："+num2str(SignalIndex));
					otherwise
						warning("收到非法信号");
				end
			end
		end
		function WaitForSignal(EW,ExpectedSignal)
			tic;
			while ~isequal(EW.Serial.read(1,"uint8"),ExpectedSignal)
				if toc>10
					tic;
					warning("Arduino设备未响应，请检查连接，或继续等待程序重试");
				end
			end
        end
	end
	methods
		function obj=ExperimentWorker
			obj.WatchDog=timer(StartDelay=10,TimerFcn=@obj.CloseSerial);
			obj.Recorder=Gbec.internal.TimeRecorder;
			obj.TrialRecorder=MATLAB.DataTypes.ArrayBuilder;
            disp(['通用行为实验控制器v' Gbec.Version().Me ' by 张天夫']);
		end
		function PauseSession(EW)
			%暂停会话
			EW.Serial.write(Gbec.UIDs.Command_Pause,"uint8");
		end
		function ContinueSession(EW)
			%继续会话
			EW.Serial.write(Gbec.UIDs.Command_Continue,"uint8");
		end
		function AbortSession(EW)
			%放弃会话
			EW.Serial.write(Gbec.UIDs.Command_Abort,"uint8");
			if ~isempty(EW.VideoInput)
				stop(EW.VideoInput);
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
		function StartCheckMonitor(EW,DeviceUID)
			%开始检查监视器
			%输入参数：DeviceUID(1,1)UIDs，设备标识符
			EW.WatchDog.stop;
			EW.Serial.write(Gbec.UIDs.Command_CheckDevice,"uint8");
			EW.FindDevice(DeviceUID);
			EW.CallbackTemp=EW.Serial.BytesAvailableFcn;
			EW.Serial.configureCallback("byte",1,@EW.MonitorCallback);
			disp("信号监测中……");
			EW.Serial.write(0,"uint8");			
		end
		function StopCheckMonitor(EW)
			%停止检查监视器
			import Gbec.UIDs
			if isempty(EW.CallbackTemp)
				EW.Serial.configureCallback("off");
			else
				EW.Serial.configureCallback("byte",1,EW.CallbackTemp);
			end
			EW.Serial.write(UIDs.Command_CheckOver,"uint8");
            EW.WaitForSignal(UIDs.Signal_TestFinished);
		end
		function OneEnterOneCheck(EW,DeviceUID,EnterPrompt)
			%检查刺激器，按一次回车给一个刺激，输入任意字符停止检查
			%输入参数：
			%DeviceUID(1,1)UIDs，设备标识符
			%EnterPrompt(1,1)string，提示文字，将显示在命令行中
			import Gbec.UIDs
			EW.WatchDog.stop;
			EW.Serial.write(UIDs.Command_SignalwiseCheck,"uint8");
			EW.FindDevice(DeviceUID);
			while input(EnterPrompt,"s")==""
				EW.Serial.write(UIDs.Command_CheckOnce,"uint8");
			end
			EW.Serial.write(UIDs.Command_CheckOver,"uint8");
		end
		function CheckManyTimes(EW,DeviceUID,CheckTimes)
			%多次检查刺激器
			%输入参数：
			%DeviceUID(1,1)UIDs，设备标识符
			%CheckTimes(1,1)uint8，检查次数
			EW.WatchDog.stop;
			EW.Serial.write(Gbec.UIDs.Command_CheckDevice,"uint8");
			EW.FindDevice(DeviceUID);
			EW.Serial.write(CheckTimes,"uint8");
		end
		function Information = GetInformation(EW)
			%获取会话信息
			%返回值：Information(1,1)struct，信息结构体
			import Gbec.UIDs
			EW.WatchDog.stop;
			EW.Serial.write(UIDs.Command_Information,"uint8");
			switch EW.Serial.read(1,"uint8")
				case UIDs.Signal_Information
					EW.WaitForSignal(UIDs.StructStart);
					Information=CollectStruct(EW.Serial);
				case UIDs.Signal_NoCurrentSession
					warning("未运行任何实验");
					Information=missing;
				otherwise
					warning("从串口返回了不正确的数据，信息收集中止");
			end
		end
		function SaveInformation(EW)
			%获取并保存会话信息
			DateTimes=table;
			DateTimes.DateTime=EW.DateTime;
			DateTimes.Mouse=EW.Mouse;
			DateTimes.Metadata={EW.GetInformation};
			Design=char(EW.Session);
			Blocks=table;
			Blocks.DateTime=EW.DateTime;
			Blocks.Design=string(Design(9:end));
			Blocks.EventLog={EW.Recorder.GetTimeTable};
			Trials=table;
			Stimulus=EW.TrialRecorder.Harvest;
			NumTrials=numel(Stimulus);
			TrialIndex=(0x001:NumTrials)';
			Trials.TrialUID=TrialIndex;
			Trials.BlockUID(:)=0x001;
			Trials.TrialIndex=TrialIndex;
			Trials.Stimulus=Stimulus;
			Version=Gbec.Version;
			save(EW.SavePath,'DateTimes','Blocks','Trials','Version');
			EW.SessionRunning=false;
			SaveDirectory=fileparts(EW.SavePath);
			disp("数据已保存到"+"<a href=""matlab:winopen('"+EW.SavePath+"');"">"+EW.SavePath+"</a> <a href=""matlab:cd('"+SaveDirectory+"');"">切换当前文件夹</a> <a href=""matlab:winopen('"+SaveDirectory+"');"">打开数据文件夹</a>");
		end
		function SerialInitialize(EW,SerialPort)
			%初始化串口
			%输入参数：SerialPort(1,1)string，串口名称
			import Gbec.UIDs
			try
				assert(EW.Serial.Port==SerialPort);
				EW.Serial.write(UIDs.Command_Abort,"uint8");
				EW.Serial.write(UIDs.Command_IsReady,"uint8");
				EW.WaitForSignal(UIDs.Signal_Ready);
			catch
				delete(EW.Serial);
				EW.Serial=serialport(SerialPort,9600);
				%刚刚初始化时不能向串口发送数据，只能等待Arduino主动宣布初始化完毕
				EW.WaitForSignal(UIDs.Signal_Ready);
			end
			EW.WatchDog.stop;
			EW.Serial.ErrorOccurredFcn=@EW.InterruptRetry;
		end
		function StartSession(EW)
			%开始会话
			import Gbec.UIDs
			HasVideo=~isempty(EW.VideoInput);
			if HasVideo
				preview(EW.VideoInput);
				start(EW.VideoInput);
				waitfor(EW.VideoInput,'Running','on');
			end
			EW.Serial.write(UIDs.Command_Start,"uint8");
			EW.Serial.write(EW.Session,"uint8");
			if EW.Serial.read(1,"uint8")==UIDs.Signal_SessionFound
				EW.WatchDog.stop;
				EW.Recorder.Reset;
				EW.SessionRunning=true;
				EW.TrialRecorder.Clear;
				disp("会话开始");
            else
                if HasVideo
                    stop(EW.VideoInput);
                end
				error("会话UID未找到");
			end
			EW.Serial.configureCallback("byte",1,@EW.GeneralCallback);
		end
	end
end