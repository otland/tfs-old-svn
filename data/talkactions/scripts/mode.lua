function onSay(cid, words, param)
	local worldtype = getWorldType()
	local msg = ""

	if worldtype == WORLD_TYPE_NO_PVP then
		msg = "No-PVP"
	elseif worldtype == WORLD_TYPE_PVP then
		msg = "PVP"
	elseif worldtype == WORLD_TYPE_PVP_ENFORCED then
		msg = "PVP-Enforced"
	else
		return TRUE
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "World type is currently set to ".. msg ..".")
	return FALSE
end