local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, 0)

local condition = createConditionObject(CONDITION_HASTE)
setConditionParam(condition, CONDITION_PARAM_TICKS, 10000)
setConditionFormula(condition, 0.8, -72, 0.8, -72)
setCombatCondition(combat, condition)

local disable = createConditionObject(CONDITION_DISABLE_ATTACK)
setConditionParam(disable, CONDITION_PARAM_TICKS, 10000)

function onCastSpell(cid, var)
	doAddCondition(cid, disable)
	return doCombat(cid, combat, var)
end
