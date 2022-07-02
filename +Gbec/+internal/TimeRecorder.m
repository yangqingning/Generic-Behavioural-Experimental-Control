classdef TimeRecorder<handle
	properties(Access=private)
		StartTime(1,1)uint64=tic
	end
	properties(GetAccess=private,SetAccess=immutable)
		TagAB MATLAB.DataTypes.ArrayBuilder
		TimeAB MATLAB.DataTypes.ArrayBuilder
	end
	methods
		function obj=TimeRecorder
			obj.TagAB=MATLAB.DataTypes.ArrayBuilder;
			obj.TimeAB=MATLAB.DataTypes.ArrayBuilder;
		end
		function Reset(obj)
			obj.StartTime=tic;
			obj.TagAB.Clear;
			obj.TimeAB.Clear;
		end
		function Hit(obj,varargin)
			if nargin>1
				obj.TagAB.Append(varargin{1});
			else
				obj.TagAB.Append("Hit");
			end
			obj.TimeAB.Append(toc(obj.StartTime));
		end
		function TimeTable=GetTimeTable(obj)
			TimeTable=timetable(seconds(obj.TimeAB.Harvest),obj.TagAB.Harvest,'VariableNames',"Tag");
		end
	end
end