function onSay(cid, words, param)
	if(getPlayerSex(cid) == 2) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You cannot change your gender.")
		return TRUE
	end

	if(getPlayerPremiumDays(cid) > 2) then
		if(getPlayerPremiumDays(cid) < 65535) then
			doPlayerAddPremiumDays(cid, -3)
		end

		if(getPlayerSex(cid) == 0) then
			doPlayerSetSex(cid, 1)
		else
			doPlayerSetSex(cid, 0)
		end

		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have changed your gender and lost three days of premium time.")
		doSendMagicEffect(getCreaturePosition(cid), CONST_ME_MAGIC_RED)
	else
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Sorry, not enough premium time- changing gender costs three days.")
		doSendMagicEffect(getCreaturePosition(cid), CONST_ME_POFF)
	end
	return TRUE
end
