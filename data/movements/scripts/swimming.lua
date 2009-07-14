local BORDERS = {
	[7943] = {x = 0, y = -2, back = SOUTH},
	[7944] = {x = -2, y = 0, back = EAST},
	[7945] = {x = 0, y = 2, back = NORTH},
	[7946] = {x = 2, y = 0, back = WEST},
	[7947] = {x = 2, y = 1, back = WEST},
	[7948] = {x = -2, y = 1, back = NORTH},
	[7949] = {x = 2, y = -1, back = WEST},
	[7950] = {x = -2, y = -1, back = WEST},
	[7951] = {x = 2, y = 2, back = WEST},
	[7952] = {x = -2, y = 2, back = NORTH},
	[7953] = {x = 2, y = -2, back = WEST},
	[7954] = {x = -2, y = -2, back = SOUTH}
}

BORDERS[4828] = BORDERS[7943]
BORDERS[4829] = BORDERS[7946]
BORDERS[4830] = BORDERS[7945]
BORDERS[4831] = BORDERS[7944]

function onStepIn(cid, item, position, fromPosition)
	if(not isPlayer(cid)) then
		return true
	end

	local border = BORDERS[item.itemid]
	if(not border) then
		return false
	end

	return checkSwim(cid, border.x, border.y, border.back)
end

local outfit = {lookType = 267, lookHead = 0, lookBody = 0, lookLegs = 0, lookFeet = 0, lookTypeEx = 0, lookAddons = 0}

function checkSwim(cid, x, y, backDir)
	local pos = getCreaturePosition(cid)
	newPos = pos
	newPos.x = pos.x + x
	newPos.y = pos.y + y

	if(hasCondition(cid, CONDITION_OUTFIT) and getCreatureOutfit(cid).lookType == outfit.lookType) then
		doMoveCreature(cid, backDir)
		doRemoveCondition(cid, CONDITION_OUTFIT)
	else
		if(queryTileAddThing(cid, pos, 4) ~= RETURNVALUE_NOERROR) then
			return false
		end

		doRemoveConditions(cid, true)

		doSendMagicEffect(pos, CONST_ME_POFF)
		doTeleportThing(cid, newPos)
		doSendMagicEffect(pos, CONST_ME_WATERSPLASH)

		doSetCreatureOutfit(cid, outfit, -1)
	end

	return true
end
