local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_HITAREA)
setCombatParam(combat, COMBAT_PARAM_USECHARGES, true)
setCombatFormula(combat, COMBAT_FORMULA_SKILL, -0.5, 0, -1.5, 0)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
