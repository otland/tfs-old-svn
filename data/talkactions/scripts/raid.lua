function onSay(cid, words, param, channel)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return TRUE
	end

	local ret = executeRaid(param)
	if(ret == LUA_ERROR) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Such raid does not exists.")
		return TRUE
	end

	if(ret == FALSE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Could not execute raid.")
		return TRUE
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Raid started.")
	return TRUE
end
