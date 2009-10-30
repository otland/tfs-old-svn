local config = {
	teleportAccess = 3
}

function onSay(cid, words, param, channel)
	if(param == '') then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return true
	end

	local teleport = false
	local t = string.explode(param, ",")
	if(t[2]) then
		teleport = getBooleanFromString(t[2])
		param = t[1]
	end

	local house = getHouseByPlayerGUID(getPlayerGUIDByName(param))
	if(not house) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. param .. " does not own house or doesn't exists.")
		return true
	end

	local houseInfo = getHouseInfo(house)
	if(teleport and getPlayerAccess(cid) >= config.teleportAccess) then
		doTeleportThing(cid, houseInfo.entry)
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, param .. " owns house: " .. houseInfo.name .. ".")
	return true
end
