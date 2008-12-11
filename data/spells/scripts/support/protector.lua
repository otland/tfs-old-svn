local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, 0)

local condition = createConditionObject(CONDITION_ATTRIBUTES)
setConditionParam(condition, CONDITION_PARAM_TICKS, 10000)
setConditionParam(condition, CONDITION_PARAM_SKILL_SHIELDPERCENT, 220)
setCombatCondition(combat, condition)

local block = createConditionObject(CONDITION_DISABLE_ATTACK)
setConditionParam(block, CONDITION_PARAM_TICKS, 10000)
setConditionParam(block, CONDITION_PARAM_BUFF, TRUE)

function onCastSpell(cid, var)
	doAddCondition(cid, block)
	return doCombat(cid, combat, var)
end
