function onSay(cid, words, param)
	if(param == "") then
		if(isNumber(param) == TRUE) then
			prepareClean(tonumber(param), cid)
		else
			doPlayerSendCancel(cid, "Command requires numeric param.")
		end
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Collected " .. cleanMap() .. " items.")
	end
	return TRUE
end

function prepareClean(minutes, cid)
	if(minutes == 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Cleaned " .. cleanMap() .. " items.")
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
