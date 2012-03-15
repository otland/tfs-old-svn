local config = {
	daysToLost = 3
}
function onSay(cid, words, param)
	if (getPlayerAccess(cid) > 0) then
		doPlayerSetSex(cid, getPlayerSex(cid) == PLAYERSEX_FEMALE and PLAYERSEX_MALE or PLAYERSEX_FEMALE)
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have changed your sex.")
		return
	end

	if (getPlayerPremiumDays(cid) < config.daysToLost) then
		doPlayerRemovePremiumDays(cid, config.daysToLost)
		doPlayerSetSex(cid, getPlayerSex(cid) == PLAYERSEX_FEMALE and PLAYERSEX_MALE or PLAYERSEX_FEMALE)
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have changed your sex and lost ".. config.daysToLost .." days of premium account.")
	else
		doPlayerSendCancel(cid, "You do not have enough premium days, changing sex costs ".. config.daysToLost .." of your premium days.")
		doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
	end
end
