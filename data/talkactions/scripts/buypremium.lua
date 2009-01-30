function onSay(cid, words, param)
	if getPlayerPremiumDays(cid) <= 360 then
		if doPlayerRemoveMoney(cid, 10000) == TRUE then
			doPlayerAddPremiumDays(cid, 90)
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have bought 90 days of premium account.")
		else
			doPlayerSendCancel(cid, "You don't have enough money, 90 days premium account costs 10000 gold coins.")
			doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
		end
	else
		doPlayerSendCancel(cid, "You can not buy more than one year of Premium Account.")
		doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
	end
	return TRUE
end
