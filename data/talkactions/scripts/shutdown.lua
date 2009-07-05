local shutdownEvent = 0

function onSay(cid, words, param, channel)
	if(param == '') then
		doSetGameState(GAMESTATE_SHUTDOWN)
		return true
	end

	if(param:lower() == "stop") then
		stopEvent(shutdownEvent)
		shutdownEvent = 0
		return true
	elseif(param:lower() == "kill") then
		os.exit()
		return true
	end

	param = tonumber(param)
	if(not param or param < 0) then
		doPlayerSendCancel(cid, "Numeric param may not be lower than 0.")
		return true
	end

	if(shutdownEvent ~= 0) then
		stopEvent(shutdownEvent)
	end

	return prepareShutdown(math.abs(math.ceil(param)))
end

function prepareShutdown(minutes)
	if(minutes <= 0) then
		doSetGameState(GAMESTATE_SHUTDOWN)
		return false
	end

	if(minutes == 1) then
		doBroadcastMessage("Server is going down in " .. minutes .. " minute, please log out now!")
	elseif(minutes <= 3) then
		doBroadcastMessage("Server is going down in " .. minutes .. " minutes, please log out.")
	else
		doBroadcastMessage("Server is going down in " .. minutes .. " minutes.")
	end

	shutdownEvent = addEvent(prepareShutdown, 60000, minutes - 1)
	return true
end
