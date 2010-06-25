local savingEvent = 0

function onSay(cid, words, param, channel)
	if(isNumber(param)) then
		stopEvent(savingEvent)
		save(tonumber(param) * 60 * 1000)
	else
		if(param == '') then
			doSaveServer()
		else
			local tid = getPlayerByNameWildcard(param)
			if(not tid or (isPlayerGhost(tid) and getPlayerGhostAccess(tid) > getPlayerGhostAccess(cid))) then
				doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. param .. " not found.")
			else
				doPlayerSave(tid)
			end
		end
	end
	return true
end

function save(delay)
	doSaveServer()
	if(delay > 0) then
		savingEvent = addEvent(save, delay, delay)
	end
end
