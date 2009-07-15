local combat = createCombatObject()
local area = createCombatArea(AREA_CROSS1X1)

setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_EXPLOSIONAREA)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_EXPLOSION)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, -0.5, 0, -0.9, 0)
setCombatArea(combat, area)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
