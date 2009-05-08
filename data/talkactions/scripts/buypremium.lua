local config = {
	days = 90,
	cost = 10000,
	maxDays = 360
}

function onSay(cid, words, param, channel)
	if(getPlayerPremiumDays(cid) > config.maxDays) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You can not buy more than " .. config.days + config.maxDays .. " days of Premium Account.")
		doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
		return TRUE
	end

	if(doPlayerRemoveMoney(cid, config.cost) ~= TRUE) then
		doPlayerSendCancel(cid, "You don't have enough money, " .. config.days .. " days premium account costs " .. config.cost .. " gold coins.")
		doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
		return TRUE
	end

	doPlayerAddPremiumDays(cid, config.days)
	doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have bought " .. config.days .. " days of premium account.")
	return TRUE
end
