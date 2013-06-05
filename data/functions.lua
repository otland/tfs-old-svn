function doPlayerGiveItem(cid, itemid, count, charges)
	local isFluidContainer = isItemFluidContainer(itemid) == TRUE
	if isFluidContainer and charges == nil then
		charges = 1
	end
	while count > 0 do
		local tempcount = 1
		if(isItemStackable(itemid) == TRUE) then
			tempcount = math.min(100, count)
		end
		local ret = doPlayerAddItem(cid, itemid, tempcount, TRUE, charges)
		if ret == false then
			ret = doCreateItem(itemid, tempcount, getPlayerPosition(cid))
		end

		if(ret) then
			if(isFluidContainer) then
				count = count - 1
			elseif isItemRune(itemid) then
				return LUA_NO_ERROR
			else
				count = count - tempcount
			end
		else
			return LUA_ERROR
		end
	end
	return LUA_NO_ERROR
end

function doCreatureSayWithRadius(cid, text, type, radiusx, radiusy, position)
	for i, tid in ipairs(getSpectators(position or getCreaturePosition(cid), radiusx, radiusy, false)) do
		if(isPlayer(tid) == TRUE) then
			doCreatureSay(cid, text, type, false, tid, position or getCreaturePosition(cid))
		end
	end
	return TRUE
end

function doSummonCreatures(monsters, positions)
	for _, positions in pairs(positions) do
		doSummonCreature(monsters, positions)
	end
end

function setPlayerMultipleStorageValues(cid, storage, value)
	for _, storage in pairs(storage) do
		setPlayerStorageValue(cid, storage, value)
	end
end

function doPlayerTakeItem(cid, itemid, count)
	if(getPlayerItemCount(cid,itemid) >= count) then
		while count > 0 do
			local tempcount = 0
			if(isItemStackable(itemid) == TRUE) then
				tempcount = math.min (100, count)
			else
				tempcount = 1
			end
			local ret = doPlayerRemoveItem(cid, itemid, tempcount)
			if(ret ~= LUA_ERROR) then
				count = count-tempcount
			else
				return LUA_ERROR
			end
		end
		if(count == 0) then
			return LUA_NO_ERROR
		end
	else
		return LUA_ERROR
	end
end

function doPlayerBuyItem(cid, itemid, count, cost, charges)
	if(doPlayerRemoveMoney(cid, cost) == TRUE) then
		return doPlayerGiveItem(cid, itemid, count, charges)
	end
	return LUA_ERROR
end

function doPlayerSellItem(cid, itemid, count, cost)
	if(doPlayerTakeItem(cid, itemid, count) == LUA_NO_ERROR) then
		if doPlayerAddMoney(cid, cost) ~= TRUE then
			error('Could not add money to ' .. getPlayerName(cid) .. '(' .. cost .. 'gp)')
		end
		return LUA_NO_ERROR
	end
	return LUA_ERROR
end

function isInRange(pos, fromPos, toPos)
	if pos.x >= fromPos.x and pos.y >= fromPos.y and pos.z >= fromPos.z and pos.x <= toPos.x and pos.y <= toPos.y and pos.z <= toPos.z then
		return TRUE
	end
	return FALSE
end

function isPremium(cid)
	return (isPlayer(cid) == TRUE and (getPlayerPremiumDays(cid) > 0 or getConfigInfo('freePremium') == "yes")) and true or false
end

function rows(result)
	return function()
		print("Deprecated function")
		return nil
	end
end

function getMonthDayEnding(day)
	if day == "01" or day == "21" or day == "31" then
		return "st"
	elseif day == "02" or day == "22" then
		return "nd"
	elseif day == "03" or day == "23" then
		return "rd"
	else
		return "th"
	end
end

function getMonthString(m)
	return os.date("%B", os.time{year = 1970, month = m, day = 1})
end

function getArticle(str)
	return str:find("[AaEeIiOoUuYy]") == 1 and "an" or "a"
end

function isNumber(str)
	return tonumber(str) ~= nil and TRUE or FALSE
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

function doPlayerAddAddons(cid, addon)
	for i = 0, #maleOutfits do
		doPlayerAddOutfit(cid, maleOutfits[i], addon)
	end

	for i = 0, #femaleOutfits do
		doPlayerAddOutfit(cid, femaleOutfits[i], addon)
	end
end

function numRows(result)
	local rows = 0
	while result:fetch() do
		rows = rows + 1
	end
	result:close()
	return rows
end

function isSorcerer(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isSorcerer: Player not found.")
		return false
	end

	return (isInArray({1,5}, getPlayerVocation(cid)) == TRUE)
end

function isDruid(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isDruid: Player not found.")
		return false
	end

	return (isInArray({2,6}, getPlayerVocation(cid)) == TRUE)
end

function isPaladin(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isPaladin: Player not found.")
		return false
	end

	return (isInArray({3,7}, getPlayerVocation(cid)) == TRUE)
end

function isKnight(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isKnight: Player not found.")
		return false
	end

	return (isInArray({4,8}, getPlayerVocation(cid)) == TRUE)
end

function getConfigInfo(info)
	if (type(info) ~= 'string') then return nil end

	dofile('config.lua')
	return _G[info]
end

function doPlayerBuyItemContainer(cid, containerid, itemid, count, cost, charges)
	if(doPlayerRemoveMoney(cid, cost) ~= TRUE) then
		return LUA_ERROR
	end

	for i = 1, count do
		local container = doCreateItemEx(containerid, 1)
		for x = 1, getContainerCapById(containerid) do
			doAddContainerItem(container, itemid, charges)
		end

		if(doPlayerAddItemEx(cid, container, TRUE) ~= RETURNVALUE_NOERROR) then
			return LUA_ERROR
		end
	end

	return LUA_NO_ERROR
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

function getPlayerLookPos(cid)
	return getPosByDir(getThingPos(cid), getPlayerLookDir(cid))
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

function getCreaturesInRange(position, radiusx, radiusy, showMonsters, showPlayers, showSummons)
	local creaturesList = {}
	for x = -radiusx, radiusx do
		for y = -radiusy, radiusy do
			if not (x == 0 and y == 0) then
				creature = getTopCreature({x = position.x+x, y = position.y+y, z = position.z})
				if (creature.type == 1 and showPlayers == 1) or (creature.type == 2 and showMonsters == 1 and (showSummons ~= 1 or (showSummons == 1 and getCreatureMaster(creature.uid) == (creature.uid)))) then
					table.insert(creaturesList, creature.uid)
				end
			end
		end
	end

	local creature = getTopCreature(position)
	if (creature.type == 1 and showPlayers == 1) or (creature.type == 2 and showMonsters == 1 and (showSummons ~= 1 or (showSummons == 1 and getCreatureMaster(creature.uid) == (creature.uid)))) then
		if not(table.find(creaturesList, creature.uid)) then
			table.insert(creaturesList, creature.uid)
		end
	end
	return creaturesList
end

function addContainerWithItems(cid, container, item, item_count, count)
	local Container = doPlayerAddItem(cid, container, 1)
	for i = 1, count do
		doAddContainerItem(Container, item, item_count)
	end
end

function tableToPos(t)
	if type(t) == "table" and #t == 3 and tonumber(table.concat(t)) then
		return {x = tonumber(t[1]), y = tonumber(t[2]), z = tonumber(t[3])}
	end
	return FALSE
end

function positionExists(pos)
 	pos.stackpos = -1
	return type(getTileThingByPos(pos)) == "table"
end

function warnPlayer(cid, msg)
	doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
	return doPlayerSendCancel(cid, msg)
end

string.split = function (str)
	local t = {}
	local function helper(word)
		table.insert(t, word)
		return ""
	end

	if(not str:gsub("%w+", helper):find("%S")) then
		return t
	end
end

string.trim = function(str)
	return (string.gsub(str, "^%s*(.-)%s*$", "%1"))
end

string.explode = function(str, sep)
	local pos, t = 1, {}
	if #sep == 0 or #str == 0 then
		return
	end

	for s, e in function() return str:find(sep, pos) end do
		table.insert(t, str:sub(pos, s - 1):trim())
		pos = e + 1
	end

	table.insert(t, str:sub(pos):trim())
	return t
end

function getCount(string)
	local b, e = string:find("%d+")
	return b and e and tonumber(string:sub(b, e)) or -1
end

 -- Returns player name if player exists in database or 0
function playerExists(name)
	dofile("./config.lua")
	if sqlType == "mysql" then
		env = assert(luasql.mysql())
		con = assert(env:connect(mysqlDatabase, mysqlUser, mysqlPass, mysqlHost, mysqlPort))
	else
		env = assert(luasql.sqlite3())
		con = assert(env:connect(sqliteDatabase))
	end

	local cur = assert(con:execute("SELECT `name` FROM `players` WHERE `name` = '" .. escapeString(name) .. "';"))
	local row = cur:fetch({}, "a")

	local name_ = ""
	if row ~= nil then
		name_ = row.name
	end

	cur:close()
	con:close()
	env:close()
	return name_
end

function isSummon(cid)
	return (isCreature(cid) == TRUE and (getCreatureMaster(cid) ~= cid)) and TRUE or FALSE
end

function isPlayerSummon(cid)
	return (isSummon(cid) == TRUE and isPlayer(getCreatureMaster(cid)) == TRUE) and TRUE or FALSE
end

function doCopyItem(item, attributes)
	local attributes = attributes or false

	local ret = doCreateItemEx(item.itemid, item.type)
	if(attributes) then
		if(item.actionid > 0) then
			doSetItemActionId(ret, item.actionid)
		end
	end

	if(isContainer(item.uid) == TRUE) then
		for i = (getContainerSize(item.uid) - 1), 0, -1 do
			local tmp = getContainerItem(item.uid, i)
			if(tmp.itemid > 0) then
				doAddContainerItemEx(ret, doCopyItem(tmp, true).uid)
			end
		end
	end

	return getThing(ret)
end

function getTibianTime()
	local worldTime = getWorldTime()
	local minutes = worldTime % 60

	local suffix = 'pm'
	if worldTime >= 720 then
		suffix = 'am'
		if worldTime >= 780 then
			worldTime = worldTime - 720
		end
	end

	local hours = math.floor(worldTime / 60)
	if minutes < 10 then
		minutes = '0' .. minutes
	end
	return hours .. ':' .. minutes .. ' ' .. suffix
end

looktypes = {
	["citizen"] = {
		looktypeMale = 128,
		looktypeFemale = 136
	},
	["hunter"] = {
		looktypeMale = 129,
		looktypeFemale = 137
	},
	["mage"] = {
		looktypeMale = 130,
		looktypeFemale = 138
	},
	["knight"] = {
		looktypeMale = 131,
		looktypeFemale = 139
	},
	["nobleman"] = {
		looktypeMale = 132,
		looktypeFemale = 140
	},
	["summoner"] = {
		looktypeMale = 133,
		looktypeFemale = 141
	},
	["warrior"] = {
		looktypeMale = 134,
		looktypeFemale = 142
	},
	["barbarian"] = {
		looktypeMale = 143,
		looktypeFemale = 147
	},
	["druid"] = {
		looktypeMale = 144,
		looktypeFemale = 148
	},
	["wizard"] = {
		looktypeMale = 145,
		looktypeFemale = 149
	},
	["oriental"] = {
		looktypeMale = 146,
		looktypeFemale = 150
	},
	["pirate"] = {
		looktypeMale = 151,
		looktypeFemale = 155
	},
	["assassin"] = {
		looktypeMale = 152,
		looktypeFemale = 156
	},
	["beggar"] = {
		looktypeMale = 153,
		looktypeFemale = 157
	},
	["shaman"] = {
		looktypeMale = 154,
		looktypeFemale = 158
	},
	["norseman"] = {
		looktypeMale = 251,
		looktypeFemale = 252
	},
	["nightmare"] = {
		looktypeMale = 268,
		looktypeFemale = 269
	},
	["jester"] = {
		looktypeMale = 273,
		looktypeFemale = 270
	},
	["brotherhood"] = {
		looktypeMale = 278,
		looktypeFemale = 279
	},
	["demonhunter"] = {
		looktypeMale = 289,
		looktypeFemale = 288
	},
	["yalaharian"] = {
		looktypeMale = 325,
		looktypeFemale = 324
	},
	["warmaster"] = {
		looktypeMale = 335,
		looktypeFemale = 336
	},

	["potionbelt"] = {
		looktypeMale = 133,
		looktypeFemale = 138
	},
	["tiara"] = {
		looktypeMale = 133,
		looktypeFemale = 138
	},
	["ferumbrashat"] = {
		looktypeMale = 130,
		looktypeFemale = 141
	},
	["wand"] = {
		looktypeMale = 130,
		looktypeFemale = 141
	}
}

function hasAddon(cid, looktype, addon)
	if looktypes[looktype] ~= nil then
		if looktype == "beggar" or looktype == "hunter" then
			if addon == 1 then
				return canPlayerWearOutfit(cid, looktypes[looktype].looktypeMale, 1) == TRUE or canPlayerWearOutfit(cid, looktypes[looktype].looktypeFemale, 2) == TRUE
			else
				return canPlayerWearOutfit(cid, looktypes[looktype].looktypeMale, 2) == TRUE or canPlayerWearOutfit(cid, looktypes[looktype].looktypeFemale, 1) == TRUE
			end
		end
		return canPlayerWearOutfit(cid, looktypes[looktype].looktypeMale, addon) == TRUE or canPlayerWearOutfit(cid, looktypes[looktype].looktypeFemale, addon) == TRUE
	end
	return false
end

function addAddon(cid, looktype, addon)
	if looktypes[looktype] ~= nil then
		if looktype == "beggar" or looktype == "hunter" then
			if addon == 1 then
				doPlayerAddOutfit(cid, looktypes[looktype].looktypeMale, 1)
				doPlayerAddOutfit(cid, looktypes[looktype].looktypeFemale, 2)
			else
				doPlayerAddOutfit(cid, looktypes[looktype].looktypeMale, 2)
				doPlayerAddOutfit(cid, looktypes[looktype].looktypeFemale, 1)
			end
		else
			doPlayerAddOutfit(cid, looktypes[looktype].looktypeMale, addon)
			doPlayerAddOutfit(cid, looktypes[looktype].looktypeFemale, addon)
		end
		doSendMagicEffect(getCreaturePosition(cid), CONST_ME_MAGIC_RED)
	end
end

table.find = function (table, value)
	for i, v in pairs(table) do
		if(v == value) then
			return i
		end
	end
	return nil
end

table.isStrIn = function (txt, str)
	for i, v in pairs(str) do
		if(txt:find(v) and not txt:find('(%w+)' .. v) and not txt:find(v .. '(%w+)')) then
			return true
		end
	end
	return false
end

function isMonsterInRange(monsterName, fromPos, toPos)
	for _x = fromPos.x, toPos.x do
		for _y = fromPos.y, toPos.y do
			for _z = fromPos.z, toPos.z do
				creature = getTopCreature({x = _x, y = _y, z = _z})
				if creature.type == 2 and getCreatureName(creature.uid):lower() == monsterName:lower() then
					return true
				end
			end
		end
	end
	return false
end

-- LuaSQL wrapper
luasql_environment = {
	connections = {}
}
function luasql_environment:new() return self end
function luasql_environment:connect()
	local connection = luasql_connection:new()
	table.insert(self.connections, connection)
	return connection
end
function luasql_environment:close()
	for _, v in pairs(self.connections) do
		v:close()
	end
	self.connections = {}
	return true
end

luasql_connection = {
	resultIds = {}
}
function luasql_connection:new() return self end
function luasql_connection:close()
	for _, v in ipairs(self.resultIds) do
		result.free(v)
	end
	self.resultIds = {}
	return true
end
function luasql_connection:execute(statement)
	if statement:sub(1, 6):upper() == "SELECT" then
		local cursor = luasql_cursor:new(self, statement)
		if cursor.resultId ~= false then
			table.insert(self.resultIds, cursor.resultId)
		end
		return cursor
	end
	return db.query(statement)
end
function luasql_connection:closedCursor(resultId)
	for k, v in ipairs(self.resultIds) do
		if v == resultId then
			table.remove(self.resultIds, k)
			break
		end
	end
end

luasql_cursor = {
	connection,
	resultId
}
function luasql_cursor:new(connection, statement)
	self.connection = connection
	self.resultId = db.storeQuery(statement)
	return self
end
function luasql_cursor:close()
	if self.resultId == false then
		return true
	end

	self.connection:closedCursor(self.resultId)
	return result.free(self.resultId)
end
function luasql_cursor:fetch()
	if self.resultId == false then
		return nil
	end

	local ret = result.getAllData(self.resultId)
	if ret == false then
		self:close()
		self.resultId = false
		return nil
	end

	if result.next(self.resultId) == false then
		self:close()
		self.resultId = false
	end
	return ret
end

luasql = {
	mysql = function() return luasql_environment:new() end,
	sqlite3 = function() return luasql_environment:new() end,
	odbc = function() return luasql_environment:new() end,
	postgres = function() return luasql_environment:new() end
}
--

function escapeString(str)
	str = db.escapeString(str)
	if str:len() <= 2 then
		return ""
	end
	return str:sub(2, str:len() - 1)
end

function createClass(parent)
	local newClass = {}
	function newClass:new(instance)
		local instance = instance or {}
		setmetatable(instance, {__index = newClass})
		return instance
	end

	if(parent ~= nil) then
		setmetatable(newClass, {__index = parent})
	end

	function newClass:getSelf()
		return newClass
	end

	function newClass:getParent()
		return baseClass
	end

	function newClass:isa(class)
		local tmp = newClass
		while(tmp ~= nil) do
			if(tmp == class) then
				return true
			end

			tmp = tmp:getParent()
		end

		return false
	end

	function newClass:setAttributes(attributes)
		for k, v in pairs(attributes) do
			newClass[k] = v
		end
	end

	return newClass
end

Result = createClass(nil)
Result:setAttributes({
	id = -1,
	query = ""
})

function Result:getID()
	return self.id
end

function Result:setID(_id)
	self.id = _id
end

function Result:getQuery()
	return self.query
end

function Result:setQuery(_query)
	self.query = _query
end

function Result:create(_query)
	self:setQuery(_query)
	local _id = db.storeQuery(self:getQuery())
	if(_id) then
		self:setID(_id)
	end

	return self:getID()
end

function Result:getRows(free)
	local free = free or false
	if(self:getID() == -1) then
		error("[Result:getRows] Result not set!")
	end

	local c = 0
	repeat
		c = c + 1
	until not self:next()

	local _query = self:getQuery()
	self:free()
	if(not free) then
		self:create(_query)
	end

	return c
end

function Result:getDataInt(s)
	if(self:getID() == -1) then
		error("[Result:getDataInt] Result not set!")
	end

	return result.getDataInt(self:getID(), s)
end

function Result:getDataLong(s)
	if(self:getID() == -1) then
		error("[Result:getDataLong] Result not set!")
	end

	return result.getDataLong(self:getID(), s)
end

function Result:getDataString(s)
	if(self:getID() == -1) then
		error("[Result:getDataString] Result not set!")
	end

	return result.getDataString(self:getID(), s)
end

function Result:getDataStream(s)
	if(self:getID() == -1) then
		error("[Result:getDataStream] Result not set!")
	end

	return result.getDataStream(self:getID(), s)
end

function Result:next()
	if(self:getID() == -1) then
		error("[Result:next] Result not set!")
	end

	return result.next(self:getID())
end

function Result:free()
	if(self:getID() == -1) then
		error("[Result:free] Result not set!")
	end

	self:setQuery("")
	local ret = result.free(self:getID())
	self:setID(-1)
	return ret
end

Result.numRows = Result.getRows
function db.getResult(query)
	if(type(query) ~= 'string') then
		return nil
	end

	local ret = Result:new()
	ret:create(query)
	return ret
end
