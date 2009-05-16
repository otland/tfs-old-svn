function onSay(cid, words, param, channel)
	local houseId = getHouseFromPos(getCreaturePosition(cid))
	if(not houseId) then
		doPlayerSendCancel(cid, "You are not inside a house.")
		doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
		return true
	end

	local owner = getHouseOwner(houseId)
	if(owner ~= getPlayerGUID(cid) and (owner ~= getPlayerGuildId(cid) or getPlayerGuildLevel(cid) ~= GUILDLEVEL_LEADER)) then
		doPlayerSendCancel(cid, "You are not the owner of this house.")
		doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
		return true
	end

	setHouseOwner(houseId, 0)
	doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have successfully left your house.")
	return true
end
