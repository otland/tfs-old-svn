local function doTargetCorpse(cid, pos)
	local getPos = pos
	getPos.stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE

	local corpse = getThingFromPos(getPos)
	if(corpse.uid > 0 and isCorpse(corpse.uid) and isMoveable(corpse.uid) and getCreatureSkullType(cid) ~= SKULL_BLACK) then
		doRemoveItem(corpse.uid)
		local creature = doCreateMonster(cid, "Skeleton", pos)
		doConvinceCreature(cid, creature)

		doSendMagicEffect(pos, CONST_ME_MAGIC_BLUE)
		return true
	end

	doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
	doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
	return false
end

function onCastSpell(cid, var)
	local pos = variantToPosition(var)
	if(pos.x ~= 0 and pos.y ~= 0) then
		return doTargetCorpse(cid, pos)
	end

	doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
	doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
	return false
end
