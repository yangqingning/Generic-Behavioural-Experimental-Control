classdef IHostAction<handle
	%设为ExperimentWorker的HostAction所必须实现的接口
	methods(Abstract)
		%Arduino发来Signal_HostAction时执行
		Run(obj,Serial,EventLogger)
		%会话结束后，将用此方法获取HostAction信息
		Info=GetInformation(obj)
	end
end