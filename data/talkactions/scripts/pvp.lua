function onSay(cid, words, param)
	local tmp = getWorldType()

	local msg = ""	
	if(tmp == WORLD_TYPE_NO_PVP) then
		msg = "No-PVP"
	elseif(tmp == WORLD_TYPE_PVP) then
		msg = "PVP"
	elseif(tmp == WORLD_TYPE_PVP_ENFORCED) then
		msg = "PVP-Enforced"
	else
		return TRUE
	end
	
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "World type is currently set to " .. msg .. ".")
	return TRUE
end
