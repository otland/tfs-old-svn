if(result == nil) then
	print("> Warning: Couldn't load database class.")
	return
end

function db.getResult(query)
	if(type(query) ~= "string") then
		return nil
	end

	local resId = db.storeQuery(query)
	local res = Result:new()
	res:setID(resId)
	return res
end

Result = {
	id = -1
}

function Result:new(o)
	local o = o or {}
	setmetatable(o, self)
	self.__index = self
	return o
end

function Result:getID()
	return self.id
end

function Result:setID(_id)
	self.id = _id
end

function Result:numRows(free)
	if(self:getID() == -1) then
		error("[Result:numRows]: Result not set!")
	end

	local c = 1
	while(self:next()) do
		c = c + 1
	end

	if(free) then
		self:free()
	end
	return c
end

function Result:getDataInt(s)
	if(self:getID() == -1) then
		error("[Result:getDataInt]: Result not set!")
	end

	return result.getDataInt(self:getID(), s)
end

function Result:getDataLong(s)
	if(self:getID() == -1) then
		error("[Result:getDataLong]: Result not set!")
	end

	return result.getDataLong(self:getID(), s)
end

function Result:getDataString(s)
	if(self:getID() == -1) then
		error("[Result:getDataString]: Result not set!")
	end

	return result.getDataString(self:getID(), s)
end

function Result:getDataStream(s)
	if(self:getID() == -1) then
		error("[Result:getDataStream]: Result not set!")
	end

	return result.getDataStream(self:getID(), s)
end

function Result:next()
	if(self:getID() == -1) then
		error("[Result:next]: Result not set!")
	end

	return result.next(self:getID())
end

function Result:free()
	if(self:getID() == -1) then
		error("[Result:free]: Result not set!")
	end

	local ret = result.free(self:getID())
	self:setID(-1)
	return ret
end