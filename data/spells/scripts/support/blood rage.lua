local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, 0)

local condition = createConditionObject(CONDITION_ATTRIBUTES)
setConditionParam(condition, CONDITION_PARAM_TICKS, 10000)
setConditionParam(condition, CONDITION_PARAM_SKILL_MELEEPERCENT, 135)
setConditionParam(condition, CONDITION_PARAM_BUFF, TRUE)
setCombatCondition(combat, condition)

local disable = createConditionObject(CONDITION_DISABLE_DEFENSE)
setConditionParam(disable, CONDITION_PARAM_TICKS, 10000)

function onCastSpell(cid, var)
	doAddCondition(cid, disable)
	return doCombat(cid, combat, var)
end
