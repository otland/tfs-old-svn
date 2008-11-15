function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return FALSE
	end

	local pid = getPlayerByNameWildcard(param)
	if(pid == 0 or (isPlayerGhost(pid) == TRUE and getPlayerAccess(pid) > getPlayerAccess(cid))) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. param .. " is not currently online.")
		return FALSE
	end

	if(getPlayerAccess(pid) >= getPlayerAccess(cid)) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You cannot kick this player.")
		return FALSE
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, getCreatureName(pid) .. " has been kicked.")
	doRemoveCreature(pid)
	return TRUE
end
