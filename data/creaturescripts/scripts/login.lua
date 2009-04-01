function onLogin(cid)
	local loss = getConfigValue('deathLostPercent')
	if(loss ~= nil) then
		loss = loss * 10
		for i = PLAYERLOSS_EXPERIENCE, PLAYERLOSS_ITEMS do
			doPlayerSetLossPercent(cid, i, loss)
		end
	end

	registerCreatureEvent(cid, "Mail")
	registerCreatureEvent(cid, "GuildMotd")
	registerCreatureEvent(cid, "PlayerDeath")
	return TRUE
end
