local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, 0)

local condition = createConditionObject(CONDITION_ATTRIBUTES)
setConditionParam(condition, CONDITION_PARAM_TICKS, 10000)
setConditionParam(condition, CONDITION_PARAM_SKILL_DISTANCEPERCENT, 150)
setCombatCondition(combat, condition)

local speed = createConditionObject(CONDITION_PARALYZE)
setConditionParam(speed, CONDITION_PARAM_TICKS, 10000)
setConditionParam(speed, CONDITION_PARAM_BUFF, TRUE)
setConditionFormula(speed, -0.7, 56, -0.7, 56)

local block = createConditionObject(CONDITION_DISABLE_DEFENSE)
setConditionParam(block, CONDITION_PARAM_TICKS, 10000)

function onCastSpell(cid, var)
	doAddCondition(cid, speed)
	doAddCondition(cid, block)
	return doCombat(cid, combat, var)
end
