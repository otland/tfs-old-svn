local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE)
setCombatParam(combat, COMBAT_PARAM_BLOCKARMOR, true)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_ETHEREALSPEAR)

function onGetFormulaValues(cid, attackSkill, attackValue, attackFactor)
	local level = getPlayerLevel(cid)
	local skill = getPlayerSkill(cid, SKILL_DISTANCE)

	return -(((skill + 25) / 3) + (level / 5)), -((skill + 25) + (level / 5))
end

setCombatCallback(combat, CALLBACK_PARAM_SKILLVALUE, "onGetFormulaValues")

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
