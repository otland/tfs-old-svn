local UP_FLOORS = {1386, 3678, 5543, 8599}
local DRAW_WELL = 1369

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(item.itemid == DRAW_WELL and item.actionid ~= 100) then
		return false
	end

	if(isInArray(UP_FLOORS, item.itemid)) then
		fromPosition.y = fromPosition.y + 1
		fromPosition.z = fromPosition.z - 1
	else
		fromPosition.z = fromPosition.z + 1
	end

	doTeleportThing(cid, fromPosition, false)
	return true
end
