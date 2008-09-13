function onSay(cid, words, param)
	if getPlayerPremiumDays(cid) > 2 then
		if getPlayerPremiumDays(cid) < 65535 then
			doPlayerAddPremiumDays(cid, -3)
		end

		if getPlayerSex(cid) == 0 then
			doPlayerSetSex(cid, 1)
		else
			doPlayerSetSex(cid, 0)
		end
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have changed your sex and lost three days of premium account.")
	else
		doPlayerSendCancel(cid, "You do not have enough premium days, changing sex costs three of your premium days.")
		doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
	end
end