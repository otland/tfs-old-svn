function onSay(cid, words, param)
	if(param == "") then
		return TRUE
	end

	doPlayerBroadcastMessage(cid, param)
	return TRUE
end
