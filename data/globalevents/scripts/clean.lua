function executeClean()
	doCleanMap()
	doBroadcastMessage("Game map cleaned, next clean in 2 hours.")
	return TRUE
end

function onThink(interval, lastExecution)
	doBroadcastMessage("Game map cleaning within 30 seconds, please pick up your items!")
	addEvent(executeClean, 30000)
	return TRUE
end