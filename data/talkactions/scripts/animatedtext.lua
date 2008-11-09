function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return FALSE
	end

	local t = string.explode(param, ",")
	if(not t[2]) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type color.")
		return FALSE
	end

	t[1] = tonumber(t[1])
	if(t[1] > 0 and t[1] < 255) then
		doSendAnimatedText(getCreaturePosition(cid), t[2], t[1])
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Typed color has to be between 0 and 255")
	end
	return TRUE
end
