classdef GratingShower<handle
	%一个示例HostAction，每次以随机的周期显示简谐黑白栅格图。你可以根据需要对它进行修改。
	properties(GetAccess=private,SetAccess=immutable)
		Rectangle
		Window
		Log2PeriodMin
	end
	properties(Access=private)
		Period
		NextImage
		CurrentVisual=0
	end
	methods
		function obj = GratingShower
			Screens=MATLAB.Graphics.Window.Screens;
			FirstNonprimary=find(~Screens.IsPrimary,1);
			if isempty(FirstNonprimary)
				FirstNonprimary=1;
			end
			obj.Window=MATLAB.Graphics.Window.Create(DeviceName=Screens.DeviceName(FirstNonprimary));
			obj.Rectangle=Screens.Rectangle(FirstNonprimary,:);
			obj.Rectangle(3:4)=obj.Rectangle(3:4)-obj.Rectangle(1:2);
			obj.Rectangle(1:2)=0;
			obj.Window.Fill([255,0,0,0]);
			obj.Log2PeriodMin=log2(single(obj.Rectangle(3)))/3;
			obj.NextImage=repmat(uint8((sin((1:Width)/((rand+1)*obj.Log2PeriodMin))+1)*255/2),4,1,obj.Rectangle(4));
		end
		function ShowNext(obj,Serial,EventLogger)
			if(obj.CurrentVisual)
				obj.Window.RemoveVisual(obj.CurrentVisual);
			end
			obj.CurrentVisual=obj.Window.Image(obj.NextImage,obj.Rectangle);
			EventLogger.LogEvent()
		end
	end
end