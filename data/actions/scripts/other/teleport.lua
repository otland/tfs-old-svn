local UP_FLOORS = {1386, 3678, 5543, 8599}
local DRAW_WELL = 1369

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(item.itemid == DRAW_WELL and item.actionid ~= 100) then
		return false
	end

	if(isInArray(UP_FLOORS, item.itemid)) then
		fromPosition.z = fromPosition.z - 1
		fromPosition.y = fromPosition.y + 1

		fromPosition.stackpos = STACKPOS_GROUND
		if(item.actionid == 100 or getThingFromPos(fromPosition).itemid == 0) then
			fromPosition.y = fromPosition.y - 2
		end
	else
		fromPosition.z = fromPosition.z + 1
	end

	doTeleportThing(cid, fromPosition, false)
	return true
end
