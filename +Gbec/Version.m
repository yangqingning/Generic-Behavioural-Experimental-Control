function V=Version
V.Me='v5.3.0';
V.MatlabExtension=MATLAB.Version;
V.MATLAB=matlabRelease().Release;
V.Deploy=4;
persistent NewVersion
if isempty(NewVersion)
	NewVersion=TextAnalytics.CheckUpdateFromGitHub('https://github.com/ShanghaitechGuanjisongLab/Generic-Behavioural-Experimental-Control/releases','通用行为实验控制',V.Me);
end