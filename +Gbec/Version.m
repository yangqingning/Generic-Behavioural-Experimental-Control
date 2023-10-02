function V=Version
V.Me='v6.0.0';
V.MatlabExtension=MATLAB.Version;
V.MATLAB=matlabRelease().Release;
V.Deploy=6;
persistent NewVersion
if isempty(NewVersion)
	NewVersion=TextAnalytics.CheckUpdateFromGitHub('https://github.com/ShanghaitechGuanjisongLab/Generic-Behavioural-Experimental-Control/releases','通用行为实验控制',V.Me);
end