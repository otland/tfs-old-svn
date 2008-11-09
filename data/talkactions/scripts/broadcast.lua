function onSay(cid, words, param)
	if(param == "") then
		return FALSE
	end

	doPlayerBroadcastMessage(cid, param)
	return TRUE
end
