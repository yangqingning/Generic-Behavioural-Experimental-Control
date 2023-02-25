function V=Version
V.Me='5.1.0';
V.MatlabExtension=MATLAB.Version;
V.MATLAB='R2022a';
V.Deploy=2;
persistent NewVersion
if isempty(NewVersion)
	NewVersion=TextAnalytics.CheckUpdateFromGitHub('https://github.com/ShanghaitechGuanjisongLab/Generic-Behavioural-Experimental-Control/releases','通用行为实验控制',V.Me);
end