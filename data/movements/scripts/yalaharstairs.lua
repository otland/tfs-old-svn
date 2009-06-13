local STAIRS_EAST = 7924
local STAIRS_NORTH = 7925

function onStepIn(cid, item, position, lastPosition, fromPosition, toPosition)
	if(fromPosition.z ~= position.z) then
		if(item.itemid == STAIRS_EAST) then
			position.y = position.y - 1
		else
			position.x = position.x - 1
		end

		doTeleportThing(cid, position, FALSE)
		return
	end

	position.z = position.z - 1
	local direction = SOUTH
	if(item.itemid == STAIRS_EAST) then
		position.y = position.y + 2
	else
		position.x = position.x + 2
		direction = EAST
	end

	doTeleportThing(cid, position, FALSE)
	doCreatureSetLookDir(cid, direction)
end
