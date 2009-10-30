local worlds = {
	[WORLD_TYPE_NO_PVP] = "No-PVP",
	[WORLD_TYPE_PVP] = "PVP",
	[WORLD_TYPE_PVP_ENFORCED] = "PVP-Enforced"
}

function onSay(cid, words, param, channel)
	local world = worlds[getWorldType()]
	if(not world) then
		return true
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "World type is currently set to " .. world .. ".")
	return true
end
