function onSay(cid, words, param)
	param = tonumber(param)
	if(param == nil or param <= 0) then
		doPlayerSendCancel(cid, "Command requires numeric param above 0.")
		return TRUE
	end

	prepareShutdown(param)
	return TRUE
end

function prepareShutdown(minutes)
	if(minutes == 0) then
		doSetGameState(GAMESTATE_SHUTDOWN)
	else
		addEvent(prepareShutdown, 60000, minutes - 1)
		if(minutes == 1) then
			doBroadcastMessage("Server is going down in " .. minutes .. " minute, please log out now!")
		elseif(minutes <= 3) then
			doBroadcastMessage("Server is going down in " .. minutes .. " minutes, please log out.")
		else
			doBroadcastMessage("Server is going down in " .. minutes .. " minutes.")
		end
	end
end
