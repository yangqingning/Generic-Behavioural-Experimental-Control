function Setup_Impl
import MATLAB.General.CopyFile
import MATLAB.General.Delete
% 不能 import System.IO.*，如果用户未安装.NET桌面运行时，将不会产生正确的错误输出
WorkingDirectory=uigetdir("","选择工作目录");
UserDirectory=fullfile(userpath,'+Gbec');
DeployDirectory=fullfile(fileparts(fileparts(fileparts(mfilename("fullpath")))),'部署');
DeployToUser=fullfile(DeployDirectory,'+Gbec');
DeployToWorking=fullfile(DeployDirectory,'工作目录');
if isfolder(UserDirectory)
	VerFile=fullfile(UserDirectory,'.ver');
	ArduinoDirectory=fullfile(UserDirectory,'Gbec');
	if isfile(VerFile)
		Fid=fopen(VerFile);
		RequestOverride=fread(Fid,1,'uint8=>uint8');
		fclose(Fid);
		Fid=fopen(fullfile(DeployToUser,'.ver'));
		RequestOverride=RequestOverride<fread(Fid,1,'uint8=>uint8');
		fclose(Fid);
		RequestOverride=isempty(RequestOverride)||RequestOverride;
	else
		RequestOverride=true;
	end
	if RequestOverride
		if input('无法保留以前版本的用户配置，是否覆盖？y/n','s')=="y"
			Delete(ArduinoDirectory);
			CopyFile(DeployToUser,userpath);
			CopyFile(fullfile(DeployToWorking,'*'),WorkingDirectory);
			Gbec.GenerateMatlabUIDs;
		else
			disp('已放弃安装');
			return
		end
	else
		OldPaths=string(System.IO.Directory.GetFiles(ArduinoDirectory,'*',System.IO.SearchOption.AllDirectories));
		[~,OldNames]=fileparts(OldPaths);
		[~,NewNames]=fileparts(string(System.IO.Directory.GetFiles(fullfile(DeployToUser,'Gbec'),'*',System.IO.SearchOption.AllDirectories)));
		[~,Index]=setdiff(OldNames,NewNames);
		Delete(OldPaths(Index));
		[FromUser,ToUser]=FilterPaths(UserDirectory,DeployToUser);
		[FromWorking,ToWorking]=FilterPaths(WorkingDirectory,DeployToWorking);
		CopyFile([FromUser,FromWorking],[ToUser,ToWorking]);
	end
elseif isfolder(WorkingDirectory)
	if input('无法保留以前版本的用户配置，是否覆盖？y/n','s')=="y"
		CopyFile(DeployToUser,userpath);
		CopyFile(fullfile(DeployToWorking,'*'),WorkingDirectory);
		Gbec.GenerateMatlabUIDs;
	else
		disp('已放弃安装');
		return
	end
else
	CopyFile(DeployToUser,userpath);
	CopyFile(fullfile(DeployToWorking,'*'),WorkingDirectory);
	Gbec.GenerateMatlabUIDs;
end
cd(WorkingDirectory);
Gbec.ArduinoCppStandard;
edit Development_Client
end
function [FromPaths,ToPaths]=FilterPaths(OldDirectory,NewDirectory)
persistent KeepNames
if isempty(KeepNames)
	KeepNames=["UID.h","UID.m","ExperimentDesign.h","Experiment_Client.mlx","SelfCheck_Client.mlx","LogTranslate.mlx"];
end
[~,OldNames,OldExtensions]=fileparts(string(System.IO.Directory.GetFiles(OldDirectory,'*',System.IO.SearchOption.AllDirectories)));
NewPaths=string(System.IO.Directory.GetFiles(NewDirectory,'*',System.IO.SearchOption.AllDirectories));
[~,NewNames,NewExtensions]=fileparts(NewPaths);
[~,Index]=setdiff(NewNames+NewExtensions,intersect(KeepNames,OldNames+OldExtensions));
FromPaths=NewPaths(Index);
ToPaths=arrayfun(@(FP)fullfile(OldDirectory,string(System.IO.Path.GetRelativePath(NewDirectory,FP))),FromPaths);
end