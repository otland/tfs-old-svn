function onSay(cid, words, param, channel)
	if(param == "") then
		return TRUE
	end

	doPlayerBroadcastMessage(cid, param)
	return TRUE
end
