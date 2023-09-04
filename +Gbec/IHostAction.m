classdef IHostAction<handle
	%设为ExperimentWorker的HostAction所必须实现的接口。
	%用户需要自行定义一个类，继承自IHostAction，并实现所有抽象方法。这样可以在实验运行中在主机上执行任何自定义动作。
	%Gbec.GratingImage是一个示例类，展示IHostAction的一种实现。
	%See also Gbec.GratingImage
	methods(Abstract)
		%测试阶段将调用此方法。可以设置自定义参数。
		%# 签名
		% ```
		% function Info=Test(obj,varargin)
		% ```
		%# 输入参数
		% obj(1,1)IHostAction
		% varargin，任何自定义参数，可以没有
		%# 返回值
		% Info，本次测试特定的返回信息，可以为任何或没有
		Info=Test(obj,varargin)
		%Arduino发来Signal_HostAction时执行
		%# 签名
		% ```
		% function Run(obj,Serial,EventLogger)
		% ```
		%# 输入参数
		% obj(1,1)IHostAction
		% Serial(1,1)serialport，串口对象。根据设计，Arduino端可能会从串口发来关于此次主机行动的额外参数，通过serialport.read方法接收参数。
		% EventLogger(1,1)MATLAB.DataTypes.EventLogger，事件记录器，可以调用其LogEvent方法并输入UID类型的参数，记录主机动作执行中发生的事件。
		%See also serialport MATLAB.DataTypes.EventLogger
		Run(obj,Serial,EventLogger)
		%会话结束后，将用此方法获取HostAction信息
		%# 签名
		% ```
		% function Info=GetInformation(obj)
		% ```
		%# 输入参数
		% obj(1,1)IHostAction
		%# 返回值
		% Info，任何关于此对象的实验信息
		Info=GetInformation(obj)
	end
end