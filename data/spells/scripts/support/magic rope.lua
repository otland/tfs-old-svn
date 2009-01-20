local spotsId = {384, 418, 8278, 8592}

function onCastSpell(cid, var)
	local pos = getCreaturePosition(cid)
	pos.stackpos = 0

	local itemGround = getThingFromPos(pos)
	if(isInArray(spotsId, itemGround.itemid) == TRUE) then
		local newPos = pos
		newPos.y = newPos.y + 1
		newPos.z = newPos.z - 1

		doTeleportThing(cid, newPos, FALSE)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
		return LUA_NO_ERROR
	else
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
		doSendMagicEffect(pos, CONST_ME_POFF)
		return LUA_ERROR
	end
end
