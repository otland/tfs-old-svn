function onSay(cid, words, param, channel)
	if(param == '') then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return true
	end

	local pid = getPlayerByNameWildcard(param)
	if(not pid) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. param .. " not found.")
		return true
	end

	if(getPlayerAccess(pid) >= getPlayerAccess(cid)) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Cannot perform action.")
		return true
	end

	local g = 1
	if(words:sub(2, 2) == "d") then
		g = -1
	end

	local newId = getPlayerGroupId(pid) + g
	if(newId <= 0 or not setPlayerGroupId(pid, newId)) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Cannot perform action.")
		return true
	end

	local str = "been " .. (g == 1 and "promoted" or "demoted") .. " to " .. getGroupInfo(newGroupId).name .. "."
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, param .. " has " .. str)
	doPlayerSendTextMessage(pid, MESSAGE_EVENT_ADVANCE, "You have " .. str)
	return true
end
