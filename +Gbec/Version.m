function V=Version
V.Me='v5.2.1';
V.MatlabExtension=MATLAB.Version;
V.MATLAB='R2022b';
V.Deploy=4;
persistent NewVersion
if isempty(NewVersion)
	NewVersion=TextAnalytics.CheckUpdateFromGitHub('https://github.com/ShanghaitechGuanjisongLab/Generic-Behavioural-Experimental-Control/releases','通用行为实验控制',V.Me);
end