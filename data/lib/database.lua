if(result == nil) then
	print("> Warning: Couldn't load database class.")
	return
end

Result = createClass(nil)
Result:setAttributes({
	id = -1
})

function Result:getID()
	return self.id
end

function Result:setID(_id)
	self.id = _id
end

function Result:getRows(free)
	if(self:getID() == -1) then
		error("[Result:getRows]: Result not set!")
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
Result.numRows = Result.getRows

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

function db.getResult(query)
	if(type(query) ~= 'string') then
		return nil
	end

	local res = Result:new()
	res:setID(db.storeQuery(query))
	return res
end
