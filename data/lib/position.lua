function isInRange(pos, fromPos, toPos)
	return (pos.x >= fromPos.x and pos.y >= fromPos.y and pos.z >= fromPos.z and pos.x <= toPos.x and pos.y <= toPos.y and pos.z <= toPos.z)
end

function getDistanceBetween(firstPosition, secondPosition)
	local xDif = math.abs(firstPosition.x - secondPosition.x)
	local yDif = math.abs(firstPosition.y - secondPosition.y)

	local posDif = math.max(xDif, yDif)
	if(firstPosition.z ~= secondPosition.z) then
		posDif = posDif + 9 + 6
	end
	return posDif
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

function comparePos(pos, posEx)
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

