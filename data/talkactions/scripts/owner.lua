function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires param.")
		return TRUE
	end

	local clean = TRUE
	local t = string.explode(param, ",")
	if(t[2]) then
		clean = getBooleanFromString(t[2])
	end

	local guid = getPlayerGUIDByName(t[1])
	local hid = getTileHouseInfo(getCreaturePosition(cid))
	if(hid and guid) then
		setHouseOwner(hid, guid, clean)
	end
	return TRUE
end
