local config = {
	days = 90,
	year = 365,
	price = 10000
}

function onSay(cid, words, param)
	if (getPlayerPremiumDays(cid) <= config.year) then
		if doPlayerRemoveMoney(cid, config.price) == TRUE then
			doPlayerAddPremiumDays(cid, config.days)
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have bought ".. config.days .." days of premium account.")
		else
			doPlayerSendCancel(cid, "You don't have enough money, ".. config.days .." days premium account costs ".. config.price .." gold coins.")
			doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
		end
	else
		doPlayerSendCancel(cid, "You can not buy more than ".. config.year .." days of Premium Account.")
		doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
	end
end