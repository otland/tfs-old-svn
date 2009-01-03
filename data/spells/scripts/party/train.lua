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
	local memberList = getPartyMembers(cid)
	if(type(memberList) ~= 'table') then
		doPlayerSendDefaultCancel(RETURNVALUE_NOPARTYMEMBERSINRANGE)
		return LUA_ERROR
	end

	local mana = (table.maxn(memberList) * 50)
	if(getPlayerMana(cid) < mana) then
		doPlayerSendDefaultCancel(RETURNVALUE_NOTENOUGHMANA)
		return LUA_ERROR
	end

	if(doCombat(cid, combat, var) == LUA_NO_ERROR) then
		local pos = variantToPosition(var)
		for _, member in ipairs(memberList) do
			if(getDistanceBetween(getCreaturePosition(member), pos) <= 36) then
				doAddCondition(member, condition)
			end
		end

		doPlayerAddManaSpent(cid, mana)
		return LUA_NO_ERROR
	end

	return LUA_ERROR
end

