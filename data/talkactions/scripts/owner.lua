local NO_OWNER_PHRASE = {"none", "nobody", "0"}

function onSay(cid, words, param, channel)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires param.")
		return TRUE
	end

	local clean = TRUE
	local t = string.explode(param, ",")
	if(t[2]) then
		clean = getBooleanFromString(t[2])
	end

	local name = tostring(t[1])
	if(not name) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid name specified.")
		return TRUE
	end

	local guid = 0
	if(not table.isStrIn(name:lower(), NO_OWNER_PHRASE)) then
		guid = getPlayerGUIDByName(name)
	end

	if(not guid) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player not found.")
		return TRUE
	end

	local hid = getTileHouseInfo(getCreaturePosition(cid))
	if(not hid) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You are not in a house.")
		return TRUE
	end

	setHouseOwner(hid, guid, clean)
	return TRUE
end
