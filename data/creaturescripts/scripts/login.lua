function onLogin(cid)
	local loss = getConfigValue('deathLostPercent')
	if(loss ~= nil) then
		for i = PLAYERLOSS_EXPERIENCE, PLAYERLOSS_ITEMS do
			doPlayerSetLossPercent(cid, i, getConfigValue('deathLostPercent'))
		end
	end

	registerCreatureEvent(cid, "PlayerDeath")
	return TRUE
end
