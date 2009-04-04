function onLogin(cid)
	local loss = getConfigValue('deathLostPercent')
	if(loss ~= nil) then
		doPlayerSetLossPercent(cid, PLAYERLOSS_EXPERIENCE, loss * 10)
	end

	registerCreatureEvent(cid, "Mail")
	registerCreatureEvent(cid, "GuildMotd")
	registerCreatureEvent(cid, "PlayerDeath")
	return TRUE
end
