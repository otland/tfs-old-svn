local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_GROUNDSHAKER)
setCombatParam(combat, COMBAT_PARAM_USECHARGES, true)

local area = createCombatArea(AREA_CIRCLE3X3)
setCombatArea(combat, area)

local WEAPON_SKILL = { SKILL_SWORD, SKILL_CLUB, SKILL_AXE, SKILL_DISTANCE, SKILL_SHIELD, SKILL_FIST }
function onGetFormulaValues(cid, level, skill, attack, factor)
	local skillTotal, levelTotal = skill + attack, level / 5
	return -(skillTotal * 0.5 + levelTotal), -(skillTotal * 1.1 + levelTotal)
end

setCombatCallback(combat, CALLBACK_PARAM_SKILLVALUE, "onGetFormulaValues")
function onCastSpell(cid, var)
	local weapon = getPlayerWeapon(cid)
	if(weapon.uid == 0) then
		return doCombat(cid, combat, var)
	end

	local info = getItemInfo(weapon.itemid)
	if(info.elementType == COMBAT_NONE) then
		return doCombat(cid, combat, var)
	end

	local result, values = doCombat(cid, combat, var), {
		onGetFormulaValues(cid, getPlayerLevel(cid), getPlayerSkillLevel(cid, WEAPON_SKILL[info.weaponType + 1]), info.elementDamage)
	}

	doCombatAreaHealth(cid, info.elementType, getThingPosition(cid), area, values[1], values[2], CONST_ME_NONE)
	return result
end
