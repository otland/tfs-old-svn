local config = {
	loginMessage = getConfigValue('loginMessage')
}

function onLogin(cid)
	local loss = getConfigValue('deathLostPercent')
	if(loss ~= nil) then
		doPlayerSetLossPercent(cid, PLAYERLOSS_EXPERIENCE, loss * 10)
	end

	local accountManager = getPlayerAccountManager(cid)
	if(accountManager == MANAGER_NONE) then
		local lastLogin, str = getPlayerLastLoginSaved(cid), config.loginMessage
		if(lastLogin > 0) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_DEFAULT, str)
			str = "Your last visit was on " .. os.date("%a %b %d %X %Y", lastLogin) .. "."
		else
			str = str .. " Please choose your outfit."
			doPlayerSendOutfitWindow(cid)
		end

		doPlayerSendTextMessage(cid, MESSAGE_STATUS_DEFAULT, str)
	else
		if(accountManager == MANAGER_NAMELOCK) then
			str = "Hello, it appears that your character has been namelocked, what would you like as your new name?"
		elseif(accountManager == MANAGER_ACCOUNT) then
			str = "Hello, type 'account' to manage your account and if you want to start over then type 'cancel'."
		elseif(accountManager == MANAGER_NEW) then
			str = "Hello, type 'account' to create an account or type 'recover' to recover an account."
		end

		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_ORANGE, str)
	end

	if(not isPlayerGhost(cid)) then
		doSendMagicEffect(getCreaturePosition(cid), CONST_ME_TELEPORT)
	end

	registerCreatureEvent(cid, "Mail")
	registerCreatureEvent(cid, "GuildMotd")
	registerCreatureEvent(cid, "PlayerDeath")
	registerCreatureEvent(cid, "Idle")
	registerCreatureEvent(cid, "SkullCheck")
	registerCreatureEvent(cid, "ReportBug")
	return true
end
