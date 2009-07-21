local UP_FLOORS = {1386, 3678, 5543, 8599, 10035}
local DRAW_WELL = 1369

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(item.itemid == DRAW_WELL and item.actionid ~= 100) then
		return false
	end

	if(isInArray(UP_FLOORS, item.itemid)) then
		fromPosition.z = fromPosition.z - 1
		fromPosition.y = fromPosition.y + 1

		fromPosition.stackpos = STACKPOS_GROUND
		if(item.actionid == 100 or getThingFromPos(fromPosition, false).itemid == 0) then
			fromPosition.y = fromPosition.y - 2
		end
	else
		fromPosition.z = fromPosition.z + 1
	end

	local pos, dir = getCreaturePosition(cid), SOUTH
	if(pos.x < fromPosition.x) then
		dir = EAST
	elseif(pos.x == fromPosition.x) then
		if(pos.y == fromPosition.y) then
			dir = getCreatureLookDirection(cid)
		elseif(pos.y > fromPosition.y) then
			dir = NORTH
		end
	elseif(pos.x > fromPosition.x) then
		dir = WEST
	end

	doTeleportThing(cid, fromPosition, false)
	doCreatureSetLookDirection(cid, dir)
	return true
end
