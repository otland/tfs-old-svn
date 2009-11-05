function onSay(cid, words, param, channel)
	if(param == '') then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return true
	end

	local t = string.explode(param, " ", 1)
	if(not t[2]) then
		doBroadcastMessage(t[1])
	elseif(not doBroadcastMessage(t[2], MESSAGE_TYPES[t[1]])) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Bad message color type.")
	end

	return true
end
