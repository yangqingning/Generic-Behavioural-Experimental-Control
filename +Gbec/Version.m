function V=Version
V.Me='v6.2.0';
V.MatlabExtension='18.1.0';
V.MATLAB='R2022b';
V.Deploy=7;
persistent NewVersion
if isempty(NewVersion)
	NewVersion=TextAnalytics.CheckUpdateFromGitHub('https://github.com/ShanghaitechGuanjisongLab/Generic-Behavioural-Experimental-Control/releases','通用行为实验控制',V.Me);
end