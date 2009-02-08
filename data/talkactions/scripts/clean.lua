function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Collected " .. doCleanMap() .. " items.")
		return TRUE
	end

	if(not tonumber(param)) then
		doPlayerSendCancel(cid, "Command requires numeric param.")
		return TRUE
	end

	prepareClean(tonumber(param), cid)
	return TRUE
end

function prepareClean(minutes, cid)
	if(minutes == 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Cleaned " .. doCleanMap() .. " items.")
		doBroadcastMessage("Game map cleaned.")
	elseif(minutes > 0) then
		if minutes == 1 then
			doBroadcastMessage("Game map cleaning in " .. minutes .. " minute, please pick up all your items.")
		else
			doBroadcastMessage("Game map cleaning in " .. minutes .. " minutes.")
		end
		addEvent(prepareClean, 60000, minutes - 1, cid)
	end
end
