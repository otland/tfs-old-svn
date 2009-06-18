function onTargetTile(cid, pos)
	local getPos = pos
	getPos.stackpos = 255

	corpse = getThingFromPos(getPos)
	if(corpse.uid > 0 and isCorpse(corpse.uid) and isMoveable(corpse.uid)) then
		doRemoveItem(corpse.uid)
		local creature = doCreateMonster(cid, "Skeleton", pos)
		doConvinceCreature(cid, creature)

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
