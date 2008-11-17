function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return TRUE
	end

	local house = getHouseByPlayerGUID(getPlayerGUIDByName(param))
	if(not house) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. param .. " does not own house or doesn't exists.")
		return TRUE
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, param .. " owns house: " .. getHouseName(house) .. ".")
	return TRUE
end
