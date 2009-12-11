function isInRange(pos, fromPosition, toPosition)
	return (position.x >= fromPosition.x and position.y >= fromPosition.y and position.z >= fromPosition.z and position.x <= toPosition.x and position.y <= toPosition.y and position.z <= toPosition.z)
end

function getDistanceBetween(firstPosition, secondPosition)
	local x, y = math.abs(firstPosition.x - secondPosition.x), math.abs(firstPosition.y - secondPosition.y)
	local diff = math.max(x, y)
	if(firstPosition.z ~= secondPosition.z) then
		diff = diff + 9 + 6
	end

	return diff
end

function getDirectionTo(pos1, pos2)
	local dir = NORTH
	if(pos1.x > pos2.x) then
		dir = WEST
		if(pos1.y > pos2.y) then
			dir = NORTHWEST
		elseif(pos1.y < pos2.y) then
			dir = SOUTHWEST
		end
	elseif(pos1.x < pos2.x) then
		dir = EAST
		if(pos1.y > pos2.y) then
			dir = NORTHEAST
		elseif(pos1.y < pos2.y) then
			dir = SOUTHEAST
		end
	else
		if(pos1.y > pos2.y) then
			dir = NORTH
		elseif(pos1.y < pos2.y) then
			dir = SOUTH
		end
	end
	return dir
end

function getCreatureLookPosition(cid)
	return getPosByDir(getThingPos(cid), getCreatureLookDirection(cid))
end

function getPosByDir(fromPosition, direction, size)
	local n = size or 1

	local pos = fromPosition
	if(direction == NORTH) then
		pos.y = pos.y - n
	elseif(direction == SOUTH) then
		pos.y = pos.y + n
	elseif(direction == WEST) then
		pos.x = pos.x - n
	elseif(direction == EAST) then
		pos.x = pos.x + n
	elseif(direction == NORTHWEST) then
		pos.y = pos.y - n
		pos.x = pos.x - n
	elseif(direction == NORTHEAST) then
		pos.y = pos.y - n
		pos.x = pos.x + n
	elseif(direction == SOUTHWEST) then
		pos.y = pos.y + n
		pos.x = pos.x - n
	elseif(direction == SOUTHEAST) then
		pos.y = pos.y + n
		pos.x = pos.x + n
	end

	return pos
end

function doComparePositions(pos, posEx)
	return pos.x == posEx.x and pos.y == posEx.y and pos.z == posEx.z
end

function getArea(pos, rangeX, rangeY)
	local t = {}
	for i = (pos.x - rangeX), (pos.x + rangeX) do
		for j = (pos.y - rangeY), (pos.y + rangeY) do
			table.insert(t, {x = i, y = j, z = pos.z})
		end
	end

	return t
end
