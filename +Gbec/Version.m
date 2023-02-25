function V=Version
V.Me='5.1.0';
V.MatlabExtension=MATLAB.Version;
V.MATLAB='R2022a';
persistent NewVersion
if isempty(NewVersion)
	try
		NewVersion=webread('https://github.com/ShanghaitechGuanjisongLab/Generic-Behavioural-Experimental-Control/releases');
	catch ME
		if any(ME.identifier==["MATLAB:webservices:ConnectionRefused","MATLAB:webservices:UnknownHost"])
			NewVersion=[];
			return;
		else
			ME.rethrow;
		end
	end
	NewVersion=char(htmlTree(NewVersion).findElement('section:first-child span.wb-break-all').extractHTMLText);
	if ~strcmp(NewVersion(2:end),V.Me)
		disp(['通用行为实验控制' NewVersion '已发布，<a href="https://github.com/ShanghaitechGuanjisongLab/Generic-Behavioural-Experimental-Control/releases">立即更新</a>']);
	end
end