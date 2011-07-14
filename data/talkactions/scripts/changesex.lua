function onSay(cid, words, param)
	if getPlayerAccess(cid) > 0 then
		doPlayerSetSex(cid, getPlayerSex(cid) == 0 and 1 or 0)
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have changed your sex.")
		return
	end

	if getPlayerPremiumDays(cid) > 2 then
		doPlayerRemovePremiumDays(cid, 3)
		doPlayerSetSex(cid, getPlayerSex(cid) == 0 and 1 or 0)
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have changed your sex and lost three days of premium account.")
	else
		doPlayerSendCancel(cid, "You do not have enough premium days, changing sex costs three of your premium days.")
		doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
	end
end
