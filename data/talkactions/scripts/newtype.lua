function onSay(cid, words, param)
	param = tonumber(param)
	if(not param) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires numeric param.")
		return FALSE
	end

	if(param < 0 or param == 1 or param == 135 or (param > 160 and param < 192) or param > 302) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "This lookType does not exist.")
		return FALSE
	end

	local tmp = getCreatureOutfit(cid)
	tmp.lookType = param
	doCreatureChangeOutfit(cid, tmp)
	return TRUE
end
