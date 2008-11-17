function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return TRUE
	end

	doSendMagicEffect(getCreaturePosition(cid), param)
	return TRUE
end
