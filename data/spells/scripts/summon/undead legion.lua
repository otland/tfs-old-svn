function onTargetTile(cid, pos)
	local getPos = pos
	getPos.stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE

	local corpse = getThingFromPos(getPos)
	if(corpse.uid > 0 and isCorpse(corpse.uid) and isMoveable(corpse.uid) and getCreatureSkullType(cid) ~= SKULL_BLACK) then
		doRemoveItem(corpse.uid)
		doConvinceCreature(cid, doCreateMonster("Skeleton", pos))

		doSendMagicEffect(pos, CONST_ME_MAGIC_BLUE)
		return true
	end

	return false
end

local area, combat = createCombatArea(AREA_CIRCLE3X3), createCombatObject()
setCombatArea(combat, area)

setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatCallback(combat, CALLBACK_PARAM_TARGETTILE, "onTargetTile")

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
