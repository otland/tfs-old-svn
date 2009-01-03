local combat = createCombatObject()
local area = createCombatArea(AREA_CROSS5X5)
setCombatArea(combat, area)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_RED)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, FALSE)

local condition = createConditionObject(CONDITION_ATTRIBUTES)
setConditionParam(condition, CONDITION_PARAM_BUFF, TRUE)
setConditionParam(condition, CONDITION_PARAM_TICKS, 2 * 60 * 1000)
setConditionParam(condition, CONDITION_PARAM_SKILL_MELEE, 3)

function onCastSpell(cid, var)
	local pos = getCreaturePosition(cid)

	local memberList = getPartyMembers(cid)
	if(type(memberList) ~= 'table') then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOPARTYMEMBERSINRANGE)
		doSendMagicEffect(pos, CONST_ME_POFF)
		return LUA_ERROR
	end

	local mana = (table.maxn(memberList) * 50)
	if(getCreatureMana(cid) < mana) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTENOUGHMANA)
		doSendMagicEffect(pos, CONST_ME_POFF)
		return LUA_ERROR
	end

	if(doCombat(cid, combat, var) == LUA_NO_ERROR) then
		for _, member in ipairs(memberList) do
			if(getDistanceBetween(getCreaturePosition(member), pos) <= 36) then
				doAddCondition(member, condition)
			end
		end

		doCreatureAddMana(cid, -mana, FALSE)
		doPlayerAddSpentMana(cid, mana)
		return LUA_NOERROR
	end

	return LUA_ERROR
end
