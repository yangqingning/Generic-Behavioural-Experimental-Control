classdef IHostAction<handle
	%设为ExperimentWorker的HostAction所必须实现的接口
	methods(Abstract)
		Run(obj,Serial,EventLogger)
		Info=GetInformation(obj)
	end
end