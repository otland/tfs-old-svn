local config = {
	broadcast = "no"
}

config.broadcast = getBooleanFromString(config.broadcast)
local function executeSave(seconds)
	if(seconds == 0) then
		doSaveServer()
		return true
	end

	if(seconds == 120 or seconds == 30) then
		doBroadcastMessage("Full server save within " .. seconds .. " seconds, please stay in safe place!")
	end

	seconds = seconds - 30
	if(seconds >= 0) then
		addEvent(executeSave, 30 * 1000, seconds)
	end
end

function onThink(interval, lastExecution, thinkInterval)
	if(not config.broadcast) then
		doSaveServer()
		return true
	end

	executeSave(120)
	return true
end
