function onSay(cid, words, param, channel)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return TRUE
	end

	local t = string.explode(param, ";")
	if(not t[2]) then
		doBroadcastMessage(t[1])
	elseif(doBroadcastMessage(t[2], MESSAGE_TYPES[t[1]]) == LUA_ERROR) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Bad message color type.")
	end

	return TRUE
end
