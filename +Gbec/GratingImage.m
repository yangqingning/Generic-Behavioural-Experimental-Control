classdef GratingImage<Gbec.IHostAction
	%示例类，用于在HostAction中根据串口指示UID.Signal_HFImage还是LFImage，显示高低频栅格图
	properties(SetAccess=immutable)
		Window
		HighFrequencyImage
		LowFrequencyImage
		Timer
	end
	properties(SetAccess=private)
		CurrentImage
	end
	methods
		function obj = GratingImage
			ScreenTable=MATLAB.Graphics.Window.Screens;
			FirstSecondary=find(~ScreenTable.IsPrimary,1);
			if isempty(FirstSecondary)
				FirstSecondary=1;
			end
			obj.Window=MATLAB.Graphics.Window.Create(DeviceName=ScreenTable.DeviceName(FirstSecondary));
			Rectangle=ScreenTable.Rectangle(FirstSecondary,:);
			obj.Window.Fill([0xff,0,0,0]);
			Width=single(Rectangle(3)-Rectangle(1));
			Height=Rectangle(4)-Rectangle(2);
			Xs=1:Width;
			obj.HighFrequencyImage=repmat(uint8((sin(Xs/Width^(1/3))+1)*255/2),4,1,Height);
			obj.LowFrequencyImage=repmat(uint8((sin(Xs/Width^(2/3))+1)*255/2),4,1,Height);
			obj.Timer=timer(StartDelay=1,TimerFcn=@(~,~)obj.Window.RemoveVisual(obj.CurrentImage));
		end
		function Run(obj,Serial,EventLogger)
			%Arduino发来HostAction信号时，执行此方法
			Signal=Gbec.UID(Serial.read(1,'uint8'));
			switch Signal
				case Gbec.UID.Signal_HFImage
					obj.CurrentImage=obj.Window.Image(obj.HighFrequencyImage);
				case Gbec.UID.Signal_LFImage
					obj.CurrentImage=obj.Window.Image(obj.LowFrequencyImage);
			end
			obj.Timer.start;
			EventLogger.LogEvent(Signal);
		end
		function Information=GetInformation(obj)
			%会话结束后，通过此方法获取信息
			Information=struct;
			Information.HighFrequencyImage=squeeze(obj.HighFrequencyImage(1,:,:));
			Information.LowFrequencyImage=squeeze(obj.LowFrequencyImage(1,:,:));
			Information.ShowDuration=seconds(1);
		end
		function delete(obj)
			delete(obj.Timer);
		end
	end
end